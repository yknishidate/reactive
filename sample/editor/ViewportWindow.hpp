#pragma once
#include <ImGuizmo.h>
#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include <glm/gtx/matrix_decompose.hpp>

#include "Scene.hpp"
#include "reactive/Graphics/Context.hpp"

struct PushConstants {
    glm::mat4 viewProj{1};
    glm::mat4 model{1};
    glm::vec3 color{1};
};

class GridRenderer {
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
            .pushSize = sizeof(Constants),
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

    void render(const rv::CommandBuffer& commandBuffer,
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

    rv::GraphicsPipelineHandle pipeline;
    rv::DescriptorSetHandle descSet;

    rv::Mesh mainGridMesh;
    rv::Mesh subGridMesh;

    Constants constants;
};

class ViewportWindow {
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

        gridRenderer.createPipeline(context);
    }

    void createIcons(const rv::Context& context) {
        icons.push_back(rv::Image::loadFromFile(context, ASSET_DIR + "icons/manip_translate.png"));
        icons.push_back(rv::Image::loadFromFile(context, ASSET_DIR + "icons/manip_rotate.png"));
        icons.push_back(rv::Image::loadFromFile(context, ASSET_DIR + "icons/manip_scale.png"));
        iconDescSets.emplace_back(ImGui_ImplVulkan_AddTexture(
            icons[0]->getSampler(), icons[0]->getView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        iconDescSets.emplace_back(ImGui_ImplVulkan_AddTexture(
            icons[1]->getSampler(), icons[1]->getView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        iconDescSets.emplace_back(ImGui_ImplVulkan_AddTexture(
            icons[2]->getSampler(), icons[2]->getView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
    }

    void editTransform(const rv::Camera& camera, glm::mat4& matrix) const {
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();

        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y,  // break
                          ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

        glm::mat4 cameraProjection = camera.getProj();
        glm::mat4 cameraView = camera.getView();
        ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
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

    void processMouseInput() {
        if (ImGui::IsWindowHovered() && !ImGuizmo::IsUsing()) {
            dragDelta.x = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x * 0.5f;
            dragDelta.y = -ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y * 0.5f;
            mouseScroll = ImGui::GetIO().MouseWheel;
        }
        ImGui::ResetMouseDragDelta();
    }

    void showGizmo(Scene& scene, Node* selectedNode, const rv::Camera& camera, int frame) const {
        for (auto& node : scene.nodes) {
            if (&node == selectedNode) {
                glm::mat4 model = node.computeTransformMatrix(frame);
                editTransform(camera, model);

                Transform& transform = node.transform;
                glm::vec3 skew;
                glm::vec4 perspective;
                glm::decompose(model, transform.scale, transform.rotation, transform.translation,
                               skew, perspective);
            }
        }
    }

    void showToolIcon(int toolIndex, float thumbnailSize, ImGuizmo::OPERATION operation) {
        ImVec4 bgColor = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 buttonMin = ImGui::GetCursorScreenPos();
        ImVec2 buttonMax = ImVec2(buttonMin.x + thumbnailSize, buttonMin.y + thumbnailSize);
        if (mousePos.x >= buttonMin.x &&  // break
            mousePos.y >= buttonMin.y &&  // break
            mousePos.x <= buttonMax.x &&  // break
            mousePos.y <= buttonMax.y) {
            bgColor = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        }

        if (ImGui::ImageButton(iconDescSets[toolIndex],         // texture
                               {thumbnailSize, thumbnailSize},  // size
                               {0, 0}, {1, 1}, 0, bgColor)) {
            currentGizmoOperation = operation;
        }
        ImGui::NextColumn();
    }

    void showToolBar(ImVec2 viewportPos) {
        ImGui::SetCursorScreenPos(ImVec2(viewportPos.x + 10, viewportPos.y + 10));
        ImGui::BeginChild("Toolbar", ImVec2(180, 60), false,
                          ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);

        float padding = 1.0f;
        float thumbnailSize = 50.0f;
        float cellSize = thumbnailSize + padding;
        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = static_cast<int>(panelWidth / cellSize);
        ImGui::Columns(columnCount, 0, false);

        showToolIcon(0, thumbnailSize, ImGuizmo::TRANSLATE);
        showToolIcon(1, thumbnailSize, ImGuizmo::ROTATE);
        showToolIcon(2, thumbnailSize, ImGuizmo::SCALE);

        ImGui::Columns(1);
        ImGui::EndChild();
    }

    void show(const rv::CommandBuffer& commandBuffer,
              Scene& scene,
              Node* selectedNode,
              const rv::Camera& camera,
              int frame) {
        if (ImGui::Begin("Viewport")) {
            processMouseInput();

            // Show viewport
            ImVec2 windowPos = ImGui::GetCursorScreenPos();
            ImVec2 windowSize = ImGui::GetContentRegionAvail();
            width = windowSize.x;
            height = windowSize.y;

            ImGui::Image(imguiDescSet, windowSize, ImVec2(0, 1), ImVec2(1, 0));

            showToolBar(windowPos);

            showGizmo(scene, selectedNode, camera, frame);

            // Render scene
            {
                commandBuffer.bindDescriptorSet(descSet, pipeline);
                commandBuffer.bindPipeline(pipeline);
                commandBuffer.clearColorImage(colorImage, {0.05f, 0.05f, 0.05f, 1.0f});
                commandBuffer.clearDepthStencilImage(depthImage, 1.0f, 0);
                commandBuffer.transitionLayout(colorImage, vk::ImageLayout::eGeneral);

                vk::Extent3D extent = colorImage->getExtent();
                commandBuffer.setViewport(extent.width, extent.height);
                commandBuffer.setScissor(extent.width, extent.height);
                commandBuffer.beginRendering(colorImage, depthImage, {0, 0},
                                             {extent.width, extent.height});

                glm::mat4 viewProj = camera.getProj() * camera.getView();
                scene.draw(commandBuffer, pipeline, viewProj, frame);
                gridRenderer.render(commandBuffer, extent.width, extent.height, viewProj);

                commandBuffer.endRendering();
            }

            ImGui::End();
        }
    }

    bool needsRecreate() const {
        vk::Extent3D extent = colorImage->getExtent();
        return static_cast<uint32_t>(width) != extent.width ||
               static_cast<uint32_t>(height) != extent.height;
    }

    glm::vec2 dragDelta = {0.0f, 0.0f};
    float mouseScroll = 0.0f;
    bool clicked = false;
    float width;
    float height;
    rv::ImageHandle colorImage;
    rv::ImageHandle depthImage;
    vk::DescriptorSet imguiDescSet;

    rv::DescriptorSetHandle descSet;
    rv::GraphicsPipelineHandle pipeline;
    GridRenderer gridRenderer;

    std::vector<rv::ImageHandle> icons;
    std::vector<vk::DescriptorSet> iconDescSets;
    ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
};
