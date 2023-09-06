
#include <reactive/App.hpp>

#include "AssetWindow.hpp"
#include "AttributeWindow.hpp"
#include "Scene.hpp"
#include "SceneWindow.hpp"
#include "ViewportWindow.hpp"

using namespace rv;

class RenderWindow {
public:
    std::string vertCode = R"(
    #version 450
    layout(location = 0) in vec3 inPosition;
    layout(location = 1) in vec3 inNormal;
    layout(location = 2) in vec2 inTexCoord;
    layout(location = 0) out vec3 outNormal;

    layout(push_constant) uniform PushConstants {
        mat4 viewProj;
        mat4 model;
        vec3 color;
    };

    void main() {
        gl_Position = viewProj * model * vec4(inPosition, 1);
        outNormal = inNormal;
    })";

    std::string fragCode = R"(
    #version 450
    layout(location = 0) in vec3 inNormal;
    layout(location = 0) out vec4 outColor;

    layout(push_constant) uniform PushConstants {
        mat4 viewProj;
        mat4 model;
        vec3 color;
    };

    void main() {
        vec3 lightDir = normalize(vec3(1, 2, -3));
        vec3 diffuse = color * (dot(lightDir, inNormal) * 0.5 + 0.5);
        outColor = vec4(diffuse, 1.0);
    })";

    void init(const rv::Context& context, uint32_t width, uint32_t height) {
        createPipeline(context);
        createImages(context, width, height);
    }

    void createPipeline(const rv::Context& context) {
        std::vector<rv::ShaderHandle> shaders(2);
        shaders[0] = context.createShader({
            .code = rv::Compiler::compileToSPV(vertCode, vk::ShaderStageFlagBits::eVertex),
            .stage = vk::ShaderStageFlagBits::eVertex,
        });

        shaders[1] = context.createShader({
            .code = rv::Compiler::compileToSPV(fragCode, vk::ShaderStageFlagBits::eFragment),
            .stage = vk::ShaderStageFlagBits::eFragment,
        });

        descSet = context.createDescriptorSet({
            .shaders = shaders,
        });

        pipeline = context.createGraphicsPipeline({
            .descSetLayout = descSet->getLayout(),
            .pushSize = sizeof(PushConstants),
            .vertexShader = shaders[0],
            .fragmentShader = shaders[1],
            .vertexStride = sizeof(rv::Vertex),
            .vertexAttributes = rv::Vertex::getAttributeDescriptions(),
            .viewport = "dynamic",
            .scissor = "dynamic",
            .colorFormat = vk::Format::eR8G8B8A8Unorm,
        });
    }

    void createImages(const rv::Context& context, uint32_t width, uint32_t height) {
        colorImage = context.createImage({
            .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage |
                     vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc |
                     vk::ImageUsageFlagBits::eColorAttachment,
            .extent = {width, height, 1},
            .format = vk::Format::eR8G8B8A8Unorm,
            .layout = vk::ImageLayout::eGeneral,
            .debugName = "RenderWindow::colorImage",
        });

        // Create desc set
        ImGui_ImplVulkan_RemoveTexture(imguiDescSet);
        imguiDescSet = ImGui_ImplVulkan_AddTexture(colorImage->getSampler(), colorImage->getView(),
                                                   VK_IMAGE_LAYOUT_GENERAL);
    }

    void show(Scene& scene, Node* selectedNode, const rv::Camera& camera, int frame) {
        if (ImGui::Begin("Render")) {
            ImVec2 windowPos = ImGui::GetCursorScreenPos();
            ImVec2 windowSize = ImGui::GetContentRegionAvail();
            vk::Extent3D imageExtent = colorImage->getExtent();
            float aspect = static_cast<float>(imageExtent.width) / imageExtent.height;

            ImVec2 imageSize;
            imageSize.x = windowSize.x;
            imageSize.y = windowSize.x / aspect;

            float padding = (windowSize.y - imageSize.y) / 2.0;
            ImGui::SetCursorScreenPos(ImVec2(windowPos.x, windowPos.y + padding));

            ImGui::Image(imguiDescSet, imageSize, ImVec2(0, 1), ImVec2(1, 0));

            ImGui::End();
        }
    }

    void drawContent(const rv::CommandBuffer& commandBuffer,
                     const Scene& scene,
                     const rv::Camera& camera,
                     int frame) {
        commandBuffer.bindDescriptorSet(descSet, pipeline);
        commandBuffer.bindPipeline(pipeline);
        commandBuffer.clearColorImage(colorImage, {1.0, 1.0, 0.5, 1.0});
        commandBuffer.transitionLayout(colorImage, vk::ImageLayout::eGeneral);

        // TODO: change camera aspect
        glm::mat4 viewProj = camera.getProj() * camera.getView();
    }

    rv::ImageHandle colorImage;
    vk::DescriptorSet imguiDescSet;

    rv::DescriptorSetHandle descSet;
    rv::GraphicsPipelineHandle pipeline;
};

class Editor : public App {
public:
    Editor()
        : App({
              .width = 2560,
              .height = 1440,
              .title = "Reactive Editor",
              .layers = {Layer::Validation},
          }) {}

    void onStart() override {
        // Add mesh
        scene.meshes.push_back(Mesh::createCubeMesh(context, {}));

        // Add material
        Material material;
        material.baseColor = glm::vec4{1, 0, 0, 1};
        material.name = "Standard 0";
        scene.materials.push_back(material);

        material.baseColor = glm::vec4{1, 1, 0, 1};
        material.name = "Standard 1";
        scene.materials.push_back(material);

        // Add node
        Node node;
        node.name = "Cube 0";
        node.mesh = &scene.meshes.back();
        node.material = &scene.materials[0];
        scene.nodes.push_back(node);

        node.name = "Cube 1";
        node.material = &scene.materials[1];
        scene.nodes.push_back(node);

        camera = OrbitalCamera{this, 1920, 1080};
        camera.fovY = glm::radians(30.0f);
        camera.distance = 10.0f;

        assetWindow.init(context);
        viewportWindow.init(context, 1920, 1080);
        renderWindow.init(context, 1920, 1080);
    }

    void onUpdate() override {
        camera.processDragDelta(viewportWindow.dragDelta);
        camera.processMouseScroll(viewportWindow.mouseScroll);
        frame++;
    }

    void onRender(const CommandBuffer& commandBuffer) override {
        if (viewportWindow.needsRecreate()) {
            context.getDevice().waitIdle();
            viewportWindow.createImages(context, viewportWindow.width, viewportWindow.height);
            camera.aspect = viewportWindow.width / viewportWindow.height;
        }
        static bool dockspaceOpen = true;
        commandBuffer.clearColorImage(getCurrentColorImage(), {0.0f, 0.0f, 0.0f, 1.0f});

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
        windowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        windowFlags |= ImGuiWindowFlags_MenuBar;

        if (dockspaceOpen) {
            ImGui::Begin("DockSpace", &dockspaceOpen, windowFlags);
            ImGui::PopStyleVar(3);

            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */
                    }
                    if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do stuff */
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Edit")) {
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Create")) {
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

            sceneWindow.show(scene, &selectedNode);
            attributeWindow.show(scene, selectedNode);
            assetWindow.show(scene);
            viewportWindow.show(scene, selectedNode, camera, frame);
            renderWindow.show(scene, selectedNode, camera, frame);

            viewportWindow.drawContent(commandBuffer, scene, camera, frame);
            renderWindow.drawContent(commandBuffer, scene, camera, frame);

            ImGui::End();
        }
    }

    // Scene
    OrbitalCamera camera;
    Scene scene;
    int frame = 0;

    // ImGui
    Node* selectedNode = nullptr;

    // Editor
    SceneWindow sceneWindow;
    ViewportWindow viewportWindow;
    AttributeWindow attributeWindow;
    AssetWindow assetWindow;
    RenderWindow renderWindow;
};

int main() {
    try {
        Editor app{};
        app.run();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}
