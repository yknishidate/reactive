
#include <reactive/App.hpp>

#include <ImGuizmo.h>
#include <imgui_impl_vulkan.h>

#include <glm/gtx/matrix_decompose.hpp>

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

        PlaneLineMeshCreateInfo gridInfo;
        gridInfo.width = 20.0f;
        gridInfo.height = 20.0f;
        gridInfo.widthSegments = 20;
        gridInfo.heightSegments = 20;
        mainGridMesh = Mesh::createPlaneLineMesh(context, gridInfo);

        gridInfo.widthSegments = 100;
        gridInfo.heightSegments = 100;
        subGridMesh = Mesh::createPlaneLineMesh(context, gridInfo);
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
        commandBuffer.drawIndexed(mainGridMesh.vertexBuffer, mainGridMesh.indexBuffer,
                                  mainGridMesh.getIndicesCount());

        constants.color = glm::vec3(0.3);
        commandBuffer.setLineWidth(1.0f);
        commandBuffer.pushConstants(pipeline, &constants);
        commandBuffer.drawIndexed(subGridMesh.vertexBuffer, subGridMesh.indexBuffer,
                                  subGridMesh.getIndicesCount());
    }

    struct Constants {
        glm::mat4 viewProj;
        glm::vec3 color;
    };

    GraphicsPipelineHandle pipeline;
    DescriptorSetHandle descSet;

    Mesh mainGridMesh;
    Mesh subGridMesh;

    Constants constants;
};

struct Material {
    int baseColorTextureIndex{-1};
    int metallicRoughnessTextureIndex{-1};
    int normalTextureIndex{-1};
    int occlusionTextureIndex{-1};
    int emissiveTextureIndex{-1};

    glm::vec4 baseColorFactor{1.0f};
    float metallicFactor{0.0f};
    float roughnessFactor{0.0f};
    glm::vec3 emissiveFactor{0.0f};
};

struct Transform {
    glm::vec3 translation = {0.0f, 0.0f, 0.0f};
    glm::quat rotation = {1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale = {1.0f, 1.0f, 1.0f};

    glm::mat4 computeTransformMatrix() const {
        glm::mat4 T = glm::translate(glm::mat4{1.0}, translation);
        glm::mat4 R = glm::mat4_cast(rotation);
        glm::mat4 S = glm::scale(glm::mat4{1.0}, scale);
        return T * R * S;
    }

    glm::mat4 computeNormalMatrix() const {
        glm::mat4 R = glm::mat4_cast(rotation);
        glm::mat4 S = glm::scale(glm::mat4{1.0}, glm::vec3{1.0} / scale);
        return R * S;
    }

    static Transform lerp(const Transform& a, const Transform& b, float t) {
        Transform transform;
        transform.translation = glm::mix(a.translation, b.translation, t);
        transform.rotation = glm::lerp(a.rotation, b.rotation, t);
        transform.scale = glm::mix(a.scale, b.scale, t);
        return transform;
    }
};

struct KeyFrame {
    int frame;
    Transform transform;
};

class Node {
public:
    Transform computeTransformAtFrame(int frame) const {
        // Handle frame out of range
        if (frame <= keyFrames.front().frame) {
            return keyFrames.front().transform;
        }
        if (frame >= keyFrames.back().frame) {
            return keyFrames.back().transform;
        }

        // Search frame
        for (int i = 0; i < keyFrames.size(); i++) {
            const auto& keyFrame = keyFrames[i];
            if (keyFrame.frame == frame) {
                return keyFrame.transform;
            }

            if (keyFrame.frame > frame) {
                const KeyFrame& prev = keyFrames[i - 1];
                const KeyFrame& next = keyFrames[i];
                float t = (frame = prev.frame) / (next.frame - prev.frame);

                return Transform::lerp(prev.transform, next.transform, t);
            }
        }
    }

    glm::mat4 computeTransformMatrix(int frame) const {
        if (keyFrames.empty()) {
            return transform.computeTransformMatrix();
        }
        return computeTransformAtFrame(frame).computeTransformMatrix();
    }

    glm::mat4 computeNormalMatrix(int frame) const {
        if (keyFrames.empty()) {
            return transform.computeNormalMatrix();
        }
        return computeTransformAtFrame(frame).computeNormalMatrix();
    }

    Mesh* mesh = nullptr;
    Material* material = nullptr;
    Transform transform;
    std::vector<KeyFrame> keyFrames;
};

class Scene {
public:
    void draw(CommandBuffer commandBuffer,
              PipelineHandle pipeline,
              const glm::mat4& viewProj,
              int frame) const {
        PushConstants pushConstants;
        pushConstants.viewProj = viewProj;

        for (auto& node : nodes) {
            pushConstants.model = node.computeTransformMatrix(frame);
            Mesh* mesh = node.mesh;
            commandBuffer.pushConstants(pipeline, &pushConstants);
            commandBuffer.drawIndexed(mesh->vertexBuffer, mesh->indexBuffer,
                                      mesh->getIndicesCount());
        }
    }

    std::vector<Node> nodes;
    std::vector<Mesh> meshes;
    std::vector<Material> materials;
    std::vector<Camera> cameras;
};

class Editor : public App {
public:
    Editor()
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

        // Add mesh
        scene.meshes.push_back(Mesh::createCubeMesh(context, {}));

        // Add material
        Material material;
        material.baseColorFactor = glm::vec4{1, 0, 0, 1};
        scene.materials.push_back(material);

        // Add node
        Node node;
        node.mesh = &scene.meshes.back();
        node.material = &scene.materials.back();
        scene.nodes.push_back(node);

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

                // Show image
                ImVec2 windowSize = ImGui::GetContentRegionAvail();
                viewportWidth = windowSize.x;
                viewportHeight = windowSize.y;
                ImGui::Image(viewportDescSet, windowSize, ImVec2(0, 1), ImVec2(1, 0));

                // Show gizmo
                if (selectedNode) {
                    glm::mat4 model = selectedNode->computeTransformMatrix(frame);
                    editTransform(camera, model);

                    Transform& transform = selectedNode->transform;
                    glm::vec3 skew;
                    glm::vec4 perspective;
                    glm::decompose(model, transform.translation, transform.rotation,
                                   transform.scale, skew, perspective);
                }

                ImGui::End();
            }

            ImGui::End();
        }
    }

    void onUpdate() override {
        camera.processDragDelta(dragDelta);
        camera.processMouseScroll(mouseScroll);
        frame++;
    }

    void onRender(const CommandBuffer& commandBuffer) override {
        vk::Extent3D extent = viewportImage->getExtent();
        if (uint32_t(viewportWidth) != extent.width || uint32_t(viewportHeight) != extent.height) {
            context.getDevice().waitIdle();

            spdlog::info("Resized: {} {}", viewportWidth, viewportHeight);
            createViewportImage(viewportWidth, viewportHeight);
            extent = viewportImage->getExtent();

            camera.aspect = viewportWidth / viewportHeight;
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

        commandBuffer.beginRendering(viewportImage, viewportDepthImage, {0, 0},
                                     {extent.width, extent.height});

        auto viewProj = camera.getProj() * camera.getView();
        scene.draw(commandBuffer, pipeline, viewProj, frame);
        gridRenderer.render(commandBuffer, extent.width, extent.height, viewProj);

        commandBuffer.endRendering();
    }

    DescriptorSetHandle descSet;
    GraphicsPipelineHandle pipeline;

    // Scene
    glm::vec2 dragDelta = {0.0f, 0.0f};
    float mouseScroll = 0.0f;
    OrbitalCamera camera;
    Scene scene;
    int frame = 0;
    Node* selectedNode = nullptr;

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
        Editor app{};
        app.run();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}
