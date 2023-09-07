#pragma once
#include <ImGuizmo.h>
#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include <glm/gtx/matrix_decompose.hpp>

#include "Scene.hpp"
#include "reactive/Graphics/Context.hpp"

class GridDrawer {
    struct PushConstants {
        glm::mat4 viewProj;
        glm::vec3 color;
    };

public:
    void createPipeline(const rv::Context& context) {
        const std::string gridVertCode = R"(
        #version 450
        layout(location = 0) in vec3 inPosition;
        layout(location = 1) in vec3 inNormal;
        layout(location = 2) in vec2 inTexCoord;
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

        std::vector<rv::ShaderHandle> shaders(2);
        shaders[0] = context.createShader({
            .code = rv::Compiler::compileToSPV(gridVertCode, vk::ShaderStageFlagBits::eVertex),
            .stage = vk::ShaderStageFlagBits::eVertex,
        });

        shaders[1] = context.createShader({
            .code = rv::Compiler::compileToSPV(gridFragCode, vk::ShaderStageFlagBits::eFragment),
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
            .topology = vk::PrimitiveTopology::eLineList,
            .polygonMode = vk::PolygonMode::eLine,
            .lineWidth = "dynamic",
        });

        rv::PlaneLineMeshCreateInfo gridInfo;
        gridInfo.width = 20.0f;
        gridInfo.height = 20.0f;
        gridInfo.widthSegments = 20;
        gridInfo.heightSegments = 20;
        mainGridMesh = rv::Mesh::createPlaneLineMesh(context, gridInfo);

        gridInfo.widthSegments = 100;
        gridInfo.heightSegments = 100;
        subGridMesh = rv::Mesh::createPlaneLineMesh(context, gridInfo);
    }

    void draw(const rv::CommandBuffer& commandBuffer,
              uint32_t width,
              uint32_t height,
              const glm::mat4& viewProj) {
        commandBuffer.setViewport(width, height);
        commandBuffer.setScissor(width, height);

        commandBuffer.bindDescriptorSet(descSet, pipeline);
        commandBuffer.bindPipeline(pipeline);

        pushConstants.viewProj = viewProj;
        pushConstants.color = glm::vec3(0.4);
        commandBuffer.setLineWidth(2.0f);
        commandBuffer.pushConstants(pipeline, &pushConstants);
        commandBuffer.drawIndexed(mainGridMesh.vertexBuffer, mainGridMesh.indexBuffer,
                                  mainGridMesh.getIndicesCount());

        pushConstants.color = glm::vec3(0.2);
        commandBuffer.setLineWidth(1.0f);
        commandBuffer.pushConstants(pipeline, &pushConstants);
        commandBuffer.drawIndexed(subGridMesh.vertexBuffer, subGridMesh.indexBuffer,
                                  subGridMesh.getIndicesCount());
    }

    rv::GraphicsPipelineHandle pipeline;
    rv::DescriptorSetHandle descSet;

    rv::Mesh mainGridMesh;
    rv::Mesh subGridMesh;

    PushConstants pushConstants;
};

class ViewportWindow {
    struct PushConstants {
        glm::mat4 viewProj;
        glm::mat4 model;
        glm::vec3 color;
    };

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

    void init(const rv::Context& context, uint32_t _width, uint32_t _height) {
        createPipeline(context);
        createImages(context, _width, _height);
        createIcons(context);

        gridDrawer.createPipeline(context);
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

    void createIcons(const rv::Context& context) {
        iconManager.addIcon(context, "manip_translate", ASSET_DIR + "icons/manip_translate.png");
        iconManager.addIcon(context, "manip_rotate", ASSET_DIR + "icons/manip_rotate.png");
        iconManager.addIcon(context, "manip_scale", ASSET_DIR + "icons/manip_scale.png");
    }

    bool editTransform(const rv::Camera& camera, glm::mat4& matrix) const {
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();

        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y,  // break
                          ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

        glm::mat4 cameraProjection = camera.getProj();
        glm::mat4 cameraView = camera.getView();
        return ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
                                    currentGizmoOperation, ImGuizmo::LOCAL, glm::value_ptr(matrix));
    }

    void createImages(const rv::Context& context, uint32_t _width, uint32_t _height) {
        width = _width;
        height = _height;

        colorImage = context.createImage({
            .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage |
                     vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc |
                     vk::ImageUsageFlagBits::eColorAttachment,
            .extent = {_width, _height, 1},
            .format = vk::Format::eR8G8B8A8Unorm,
            .layout = vk::ImageLayout::eGeneral,
            .debugName = "ViewportWindow::colorImage",
        });

        // Create desc set
        ImGui_ImplVulkan_RemoveTexture(imguiDescSet);
        imguiDescSet = ImGui_ImplVulkan_AddTexture(colorImage->getSampler(), colorImage->getView(),
                                                   VK_IMAGE_LAYOUT_GENERAL);

        depthImage = context.createImage({
            .usage = rv::ImageUsage::DepthAttachment,
            .extent = {_width, _height, 1},
            .format = vk::Format::eD32Sfloat,
            .layout = vk::ImageLayout::eDepthAttachmentOptimal,
            .aspect = vk::ImageAspectFlagBits::eDepth,
            .debugName = "ViewportWindow::depthImage",
        });
    }

    bool processMouseInput() {
        bool changed = false;
        if (ImGui::IsWindowFocused() && !ImGuizmo::IsUsing()) {
            dragDelta.x = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x * 0.5f;
            dragDelta.y = -ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y * 0.5f;
            mouseScroll = ImGui::GetIO().MouseWheel;
            if (dragDelta.x != 0.0 || dragDelta.y != 0.0 || mouseScroll != 0) {
                changed = true;
            }
        }
        ImGui::ResetMouseDragDelta();
        return changed;
    }

    bool showGizmo(Scene& scene, Node* selectedNode, const rv::Camera& camera, int frame) const {
        bool changed = false;
        for (auto& node : scene.nodes) {
            if (&node == selectedNode) {
                glm::mat4 model = node.computeTransformMatrix(frame);
                changed |= editTransform(camera, model);

                Transform& transform = node.transform;
                glm::vec3 skew;
                glm::vec4 perspective;
                glm::decompose(model, transform.scale, transform.rotation, transform.translation,
                               skew, perspective);
            }
        }
        return changed;
    }

    void showToolIcon(const std::string& name, float thumbnailSize, ImGuizmo::OPERATION operation) {
        ImVec4 bgColor = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
        if (currentGizmoOperation == operation || iconManager.isHover(thumbnailSize)) {
            bgColor = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        }
        iconManager.show(name, "", thumbnailSize, bgColor,
                         [&]() { currentGizmoOperation = operation; });
    }

    void showToolBar(ImVec2 viewportPos) {
        ImGui::SetCursorScreenPos(ImVec2(viewportPos.x + 10, viewportPos.y + 15));
        ImGui::BeginChild("Toolbar", ImVec2(180, 60), false,
                          ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);

        float padding = 1.0f;
        float thumbnailSize = 50.0f;
        float cellSize = thumbnailSize + padding;
        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = static_cast<int>(panelWidth / cellSize);
        ImGui::Columns(columnCount, 0, false);

        showToolIcon("manip_translate", thumbnailSize, ImGuizmo::TRANSLATE);
        showToolIcon("manip_rotate", thumbnailSize, ImGuizmo::ROTATE);
        showToolIcon("manip_scale", thumbnailSize, ImGuizmo::SCALE);

        ImGui::Columns(1);
        ImGui::EndChild();
    }

    int show(Scene& scene, Node* selectedNode, const rv::Camera& camera, int frame) {
        int message = Message::None;
        if (ImGui::Begin("Viewport")) {
            if (processMouseInput()) {
                message |= Message::CameraChanged;
            }

            ImVec2 windowPos = ImGui::GetCursorScreenPos();
            ImVec2 windowSize = ImGui::GetContentRegionAvail();
            width = windowSize.x;
            height = windowSize.y;
            ImGui::Image(imguiDescSet, windowSize, ImVec2(0, 1), ImVec2(1, 0));

            showToolBar(windowPos);
            if (showGizmo(scene, selectedNode, camera, frame)) {
                message |= Message::TransformChanged;
            }

            ImGui::End();
        }
        return message;
    }

    void drawContent(const rv::CommandBuffer& commandBuffer,
                     const Scene& scene,
                     const rv::Camera& camera,
                     int frame) {
        commandBuffer.bindDescriptorSet(descSet, pipeline);
        commandBuffer.bindPipeline(pipeline);
        commandBuffer.clearColorImage(colorImage, {0.05f, 0.05f, 0.05f, 1.0f});
        commandBuffer.clearDepthStencilImage(depthImage, 1.0f, 0);
        commandBuffer.transitionLayout(colorImage, vk::ImageLayout::eGeneral);

        vk::Extent3D extent = colorImage->getExtent();
        commandBuffer.setViewport(extent.width, extent.height);
        commandBuffer.setScissor(extent.width, extent.height);
        commandBuffer.beginRendering(colorImage, depthImage, {0, 0}, {extent.width, extent.height});

        // Draw scene
        glm::mat4 viewProj = camera.getProj() * camera.getView();
        pushConstants.viewProj = viewProj;
        for (auto& node : scene.nodes) {
            pushConstants.model = node.computeTransformMatrix(frame);
            pushConstants.color = node.material->baseColor.xyz;
            rv::Mesh* mesh = node.mesh;
            if (mesh) {
                commandBuffer.pushConstants(pipeline, &pushConstants);
                commandBuffer.drawIndexed(mesh->vertexBuffer, mesh->indexBuffer,
                                          mesh->getIndicesCount());
            }
        }

        // Draw grid
        gridDrawer.draw(commandBuffer, extent.width, extent.height, viewProj);

        commandBuffer.endRendering();
    }

    bool needsRecreate() const {
        vk::Extent3D extent = colorImage->getExtent();
        return static_cast<uint32_t>(width) != extent.width ||
               static_cast<uint32_t>(height) != extent.height;
    }

    glm::vec2 dragDelta = {0.0f, 0.0f};
    float mouseScroll = 0.0f;
    float width;
    float height;
    rv::ImageHandle colorImage;
    rv::ImageHandle depthImage;
    vk::DescriptorSet imguiDescSet;

    rv::DescriptorSetHandle descSet;
    rv::GraphicsPipelineHandle pipeline;
    GridDrawer gridDrawer;
    PushConstants pushConstants;

    IconManager iconManager;
    ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
};
