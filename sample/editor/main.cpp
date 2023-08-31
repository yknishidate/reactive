
#include <reactive/App.hpp>

#include <ImGuizmo.h>
#include <imgui_impl_vulkan.h>

using namespace rv;

struct PushConstants {
    glm::mat4 viewProj{1};
    glm::mat4 model{1};
};

std::string vertCode = R"(
#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 0) out vec3 outNormal;

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    mat4 model;
};

void main() {
    gl_Position = viewProj * model * vec4(inPosition, 1);
    outNormal = inNormal;
})";

std::string fragCode = R"(
#version 450
layout(location = 0) in vec3 inNormal;
layout(location = 0) out vec4 outColor;
void main() {
    vec3 lightDir = normalize(vec3(1, 2, -3));
    float diffuse = dot(lightDir, inNormal) * 0.5 + 0.5;
    outColor = vec4(vec3(max(0.0, diffuse)), 1.0);
    //outColor = vec4(inNormal, 1.0);
})";

class GridRenderer {
public:
    GridRenderer(const Context& context) {
        const std::string gridVertCode = R"(
        #version 450
        layout(location = 0) in vec3 inPosition;
        layout(push_constant) uniform PushConstants {
            mat4 viewProj;
            vec3 color;
        };

        void main() {
            gl_Position = viewProj * vec4(inPosition, 1);
            gl_PointSize = 5.0;
        })";

        const std::string gridFragCode = R"(
        #version 450
        layout(location = 0) out vec4 outColor;
        layout(push_constant) uniform PushConstants {
            mat4 viewProj;
            vec3 color;
        };

        void main() {
            outColor = vec4(color, 1.0);
        })";

        std::vector<ShaderHandle> shaders(2);
        shaders[0] = context.createShader({
            .code = Compiler::compileToSPV(gridVertCode, vk::ShaderStageFlagBits::eVertex),
            .stage = vk::ShaderStageFlagBits::eVertex,
        });

        shaders[1] = context.createShader({
            .code = Compiler::compileToSPV(gridFragCode, vk::ShaderStageFlagBits::eFragment),
            .stage = vk::ShaderStageFlagBits::eFragment,
        });

        descSet = context.createDescriptorSet({
            .shaders = shaders,
        });

        pipeline = context.createGraphicsPipeline({
            .descSetLayout = descSet->getLayout(),
            .pushSize = sizeof(Constants),
            .vertexShader = shaders[0],
            .fragmentShader = shaders[1],
            .vertexStride = sizeof(Vertex),
            .vertexAttributes = Vertex::getAttributeDescriptions(),
            .viewport = "dynamic",
            .scissor = "dynamic",
            .colorFormat = vk::Format::eR8G8B8A8Unorm,
            .topology = vk::PrimitiveTopology::eLineList,
            .polygonMode = vk::PolygonMode::eLine,
            .lineWidth = "dynamic",
        });

        mainGridMesh = context.createPlaneLineMesh({
            .width = 20.0f,
            .height = 20.0f,
            .widthSegments = 20,
            .heightSegments = 20,
        });

        subGridMesh = context.createPlaneLineMesh({
            .width = 20.0f,
            .height = 20.0f,
            .widthSegments = 100,
            .heightSegments = 100,
        });
    }

    void render(const CommandBuffer& commandBuffer,
                uint32_t width,
                uint32_t height,
                const glm::mat4& viewProj) {
        commandBuffer.setViewport(width, height);
        commandBuffer.setScissor(width, height);

        commandBuffer.bindDescriptorSet(descSet, pipeline);
        commandBuffer.bindPipeline(pipeline);

        constants.viewProj = viewProj;
        constants.color = glm::vec3(0.6);
        commandBuffer.setLineWidth(2.0f);
        commandBuffer.pushConstants(pipeline, &constants);
        commandBuffer.drawIndexed(mainGridMesh);

        constants.color = glm::vec3(0.3);
        commandBuffer.setLineWidth(1.0f);
        commandBuffer.pushConstants(pipeline, &constants);
        commandBuffer.drawIndexed(subGridMesh);
    }

    struct Constants {
        glm::mat4 viewProj;
        glm::vec3 color;
    };

    GraphicsPipelineHandle pipeline;
    DescriptorSetHandle descSet;

    MeshHandle mainGridMesh;
    MeshHandle subGridMesh;

    Constants constants;
};

class HelloApp : public App {
public:
    HelloApp()
        : App({
              .width = 2560,
              .height = 1440,
              .title = "Reactive Editor",
              .layers = {Layer::Validation},
          }),
          gridRenderer(context) {}

    void onStart() override {
        std::vector<ShaderHandle> shaders(2);
        shaders[0] = context.createShader({
            .code = Compiler::compileToSPV(vertCode, vk::ShaderStageFlagBits::eVertex),
            .stage = vk::ShaderStageFlagBits::eVertex,
        });

        shaders[1] = context.createShader({
            .code = Compiler::compileToSPV(fragCode, vk::ShaderStageFlagBits::eFragment),
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
            .vertexStride = sizeof(Vertex),
            .vertexAttributes = Vertex::getAttributeDescriptions(),
            .viewport = "dynamic",
            .scissor = "dynamic",
            .colorFormat = vk::Format::eR8G8B8A8Unorm,
        });

        cubeMesh = context.createCubeMesh({});

        camera = OrbitalCamera{this, 1920, 1080};

        viewportWidth = 1920;
        viewportHeight = 1080;
        createViewportImage(viewportWidth, viewportHeight);
    }

    void createViewportImage(uint32_t width, uint32_t height) {
        viewportImage = context.createImage({
            .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage |
                     vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc |
                     vk::ImageUsageFlagBits::eColorAttachment,
            .extent = {width, height, 1},
            .format = vk::Format::eR8G8B8A8Unorm,
            .layout = vk::ImageLayout::eGeneral,
        });
        ImGui_ImplVulkan_RemoveTexture(viewportDescSet);
        viewportDescSet = ImGui_ImplVulkan_AddTexture(
            viewportImage->getSampler(), viewportImage->getView(), VK_IMAGE_LAYOUT_GENERAL);

        viewportDepthImage = context.createImage({
            .usage = ImageUsage::DepthAttachment,
            .extent = {width, height, 1},
            .format = vk::Format::eD32Sfloat,
            .layout = vk::ImageLayout::eDepthAttachmentOptimal,
        });
    }

    void editTransform(const Camera& camera, glm::mat4& matrix) {
        // Gizmos
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();

        float windowWidth = ImGui::GetWindowWidth();
        float windowHeight = ImGui::GetWindowHeight();
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y,  // break
                          windowWidth, windowHeight);

        const glm::mat4& cameraProjection = camera.getProj();
        const glm::mat4& cameraView = camera.getView();
        ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
                             ImGuizmo::TRANSLATE, ImGuizmo::LOCAL, glm::value_ptr(matrix), nullptr,
                             nullptr);
    }

    void showFullscreenDockspace() {
        static bool dockspaceOpen = true;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        window_flags |= ImGuiWindowFlags_MenuBar;

        if (dockspaceOpen) {
            ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
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

            {
                ImGui::Begin("Scene");
                if (ImGui::TreeNode("Some object 0")) {
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Some object 1")) {
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Some object 2")) {
                    ImGui::TreePop();
                }
                ImGui::End();
            }

            static float param0 = 0.0f;
            static float param1 = 0.0f;
            static float param2 = 0.0f;
            static float color[3] = {0.0f, 0.0f, 0.0f};
            {
                ImGui::Begin("Attribute");
                ImGui::SliderFloat("Some parameter 0", &param0, 0.0f, 1.0f);
                ImGui::SliderFloat("Some parameter 1", &param1, 0.0f, 1.0f);
                ImGui::SliderFloat("Some parameter 2", &param2, 0.0f, 1.0f);
                ImGui::ColorEdit3("Some color", color);
                ImGui::End();
            }

            if (ImGui::Begin("Project")) {
                for (int n = 0; n < 5; n++)
                    ImGui::Text("Asset");
                ImGui::End();
            }

            if (ImGui::Begin("Viewport")) {
                if (ImGui::IsWindowHovered() && !ImGuizmo::IsUsing()) {
                    dragDelta.x = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x * 0.5;
                    dragDelta.y = -ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y * 0.5;
                    mouseScroll = ImGui::GetIO().MouseWheel;
                    spdlog::info("mouseScroll: {}", mouseScroll);
                }
                ImGui::ResetMouseDragDelta();

                ImVec2 windowSize = ImGui::GetContentRegionAvail();
                viewportWidth = windowSize.x;
                viewportHeight = windowSize.y;
                ImGui::Image(viewportDescSet, windowSize, ImVec2(0, 1), ImVec2(1, 0));

                editTransform(camera, pushConstants.model);

                ImGui::End();
            }

            ImGui::End();
        }
    }

    void onUpdate() override {
        camera.processDragDelta(dragDelta);
        camera.processMouseScroll(mouseScroll);
        pushConstants.viewProj = camera.getProj() * camera.getView();
    }

    void onRender(const CommandBuffer& commandBuffer) override {
        vk::Extent3D extent = viewportImage->getExtent();
        if (uint32_t(viewportWidth) != extent.width || uint32_t(viewportHeight) != extent.height) {
            context.getDevice().waitIdle();

            spdlog::info("Resized: {} {}", viewportWidth, viewportHeight);
            createViewportImage(viewportWidth, viewportHeight);
            extent = viewportImage->getExtent();

            camera.aspect = viewportWidth / viewportHeight;
            pushConstants.viewProj = camera.getProj() * camera.getView();
        }

        showFullscreenDockspace();

        commandBuffer.clearColorImage(viewportImage, {0.0f, 0.0f, 0.0f, 1.0f});
        commandBuffer.transitionLayout(viewportImage, vk::ImageLayout::eGeneral);

        commandBuffer.clearColorImage(getCurrentColorImage(), {0.0f, 0.0f, 0.0f, 1.0f});
        commandBuffer.clearDepthStencilImage(viewportDepthImage, 1.0f, 0);

        commandBuffer.setViewport(extent.width, extent.height);
        commandBuffer.setScissor(extent.width, extent.height);

        commandBuffer.bindDescriptorSet(descSet, pipeline);
        commandBuffer.bindPipeline(pipeline);
        commandBuffer.pushConstants(pipeline, &pushConstants);

        commandBuffer.beginRendering(viewportImage, viewportDepthImage, {0, 0},
                                     {extent.width, extent.height});

        commandBuffer.drawIndexed(cubeMesh);

        gridRenderer.render(commandBuffer, extent.width, extent.height, pushConstants.viewProj);

        commandBuffer.endRendering();
    }

    DescriptorSetHandle descSet;
    GraphicsPipelineHandle pipeline;
    PushConstants pushConstants;

    // Scene
    glm::vec2 dragDelta = {0.0f, 0.0f};
    float mouseScroll = 0.0f;
    OrbitalCamera camera;
    MeshHandle cubeMesh;

    // ImGui
    bool viewportClicked = false;
    float viewportWidth;
    float viewportHeight;
    // TODO: change to vector(3)
    ImageHandle viewportImage;
    ImageHandle viewportDepthImage;
    vk::DescriptorSet viewportDescSet;

    // Editor
    GridRenderer gridRenderer;
};

int main() {
    try {
        HelloApp app{};
        app.run();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}
