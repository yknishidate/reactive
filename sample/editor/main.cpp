
#include <reactive/App.hpp>

#include "AssetWindow.hpp"
#include "AttributeWindow.hpp"
#include "Scene.hpp"
#include "SceneWindow.hpp"
#include "ViewportWindow.hpp"
#include "shader/share.h"

using namespace rv;

class RenderWindow {
public:
    void init(const rv::Context& context, uint32_t width, uint32_t height) {
        createImages(context, width, height);

        vkCommandBuffer = context.allocateCommandBuffer();

        vk::FenceCreateInfo fenceInfo;
        fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
        fence = context.getDevice().createFenceUnique(fenceInfo);
        iconManager.addIcon(context, "render_ipr", ASSET_DIR + "icons/render_ipr.png");
    }

    void createPipeline(const rv::Context& context) {
        std::vector<rv::ShaderHandle> shaders(3);
        shaders[0] = context.createShader({
            .code = rv::Compiler::compileToSPV(SHADER_DIR + "base.rgen"),
            .stage = vk::ShaderStageFlagBits::eRaygenKHR,
        });

        shaders[1] = context.createShader({
            .code = rv::Compiler::compileToSPV(SHADER_DIR + "base.rmiss"),
            .stage = vk::ShaderStageFlagBits::eMissKHR,
        });

        shaders[2] = context.createShader({
            .code = rv::Compiler::compileToSPV(SHADER_DIR + "base.rchit"),
            .stage = vk::ShaderStageFlagBits::eClosestHitKHR,
        });

        descSet = context.createDescriptorSet({
            .shaders = shaders,
            .images = {{"baseImage", colorImage}},
            .accels = {{"topLevelAS", topAccel}},
        });

        pipeline = context.createRayTracingPipeline({
            .rgenShaders = shaders[0],
            .missShaders = shaders[1],
            .chitShaders = shaders[2],
            .descSetLayout = descSet->getLayout(),
            .pushSize = sizeof(RenderPushConstants),
            .maxRayRecursionDepth = 16,
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

    void showToolBar() {
        float panelWidth = ImGui::GetContentRegionAvail().x;
        float thumbnailSize = 50.0f;
        ImGui::BeginChild("Toolbar", ImVec2(panelWidth, 60), false, ImGuiWindowFlags_NoBackground);
        ImGui::Columns(1, 0, true);

        ImVec4 bgColor = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
        if (running || iconManager.isHover(thumbnailSize)) {
            bgColor = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
        }
        iconManager.show("render_ipr", "", thumbnailSize, bgColor, [&]() {
            if (running) {
                running = false;
                spdlog::info("[UI] Stop IPR");
            } else {
                running = true;
                spdlog::info("[UI] Start IPR");
            }
        });

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::EndChild();
    }

    void show(Scene& scene, const rv::Camera& camera, int frame) {
        if (ImGui::Begin("Render")) {
            showToolBar();

            ImVec2 windowSize = ImGui::GetContentRegionAvail();
            ImVec2 windowPos = ImGui::GetCursorScreenPos();
            vk::Extent3D imageExtent = colorImage->getExtent();
            float imageAspect = static_cast<float>(imageExtent.width) / imageExtent.height;
            float windowAspect = windowSize.x / windowSize.y;

            // Fit image size and move image position
            ImVec2 imageSize;
            if (imageAspect >= windowAspect) {
                imageSize.x = windowSize.x;
                imageSize.y = windowSize.x / imageAspect;

                float padding = (windowSize.y - imageSize.y) / 2.0;
                ImGui::SetCursorScreenPos(ImVec2(windowPos.x, windowPos.y + padding));
            } else {
                imageSize.y = windowSize.y;
                imageSize.x = windowSize.y * imageAspect;

                float padding = (windowSize.x - imageSize.x) / 2.0;
                ImGui::SetCursorScreenPos(ImVec2(windowPos.x + padding, windowPos.y));
            }

            ImGui::Image(imguiDescSet, imageSize, ImVec2(0, 1), ImVec2(1, 0));

            ImGui::End();
        }
    }

    void updatePushConstants(rv::Camera& camera) {
        vk::Extent3D imageExtent = colorImage->getExtent();
        float tmpAspect = camera.aspect;
        camera.aspect = static_cast<float>(imageExtent.width) / imageExtent.height;
        pushConstants.invView = camera.getInvView();
        pushConstants.invProj = camera.getInvProj();
        pushConstants.instanceDataAddress = instanceDataBuffer->getAddress();
        camera.aspect = tmpAspect;
    }

    void loadScene(const Context& context, const Scene& scene, rv::Camera& camera) {
        createInstanceDataBuffer(context, scene);
        updateInstanceDataBuffer(context, scene);
        buildAccels(context, scene);
        createPipeline(context);
    }

    void render(const Context& context, const Scene& scene, rv::Camera& camera, int frame) {
        if (!running) {
            return;
        }

        if (!topAccel) {
            loadScene(context, scene, camera);
        }

        // Ignore the request if rendering is in progress.
        vk::Result fenceStatus = context.getDevice().getFenceStatus(*fence);
        if (fenceStatus == vk::Result::eNotReady) {
            spdlog::info("RenderWindow::render(): Rendering is in progress.");
            return;
        }
        context.getDevice().resetFences(*fence);

        updatePushConstants(camera);

        vk::CommandBufferBeginInfo beginInfo;
        vkCommandBuffer->begin(beginInfo);

        vk::Extent3D imageExtent = colorImage->getExtent();
        rv::CommandBuffer commandBuffer{&context, *vkCommandBuffer};
        commandBuffer.bindDescriptorSet(descSet, pipeline);
        commandBuffer.bindPipeline(pipeline);
        commandBuffer.clearColorImage(colorImage, {1.0, 1.0, 0.5, 1.0});
        commandBuffer.transitionLayout(colorImage, vk::ImageLayout::eGeneral);
        commandBuffer.pushConstants(pipeline, &pushConstants);
        commandBuffer.traceRays(pipeline, imageExtent.width, imageExtent.height, imageExtent.depth);

        vkCommandBuffer->end();
        context.submit(*vkCommandBuffer, *fence);
    }

    void updateInstanceDataBuffer(const Context& context, const Scene& scene) {
        for (int i = 0; i < scene.nodes.size(); i++) {
            const Node& node = scene.nodes[i];
            Material* material = node.material;

            InstanceData& data = instanceData[i];
            data.baseColor = material->baseColor;
            data.metallic = material->metallic;
            data.roughness = material->roughness;
            data.emissive = material->emissive;
            data.baseColorTextureIndex = material->baseColorTextureIndex;
            data.emissiveTextureIndex = material->emissiveTextureIndex;
            data.metallicRoughnessTextureIndex = material->metallicRoughnessTextureIndex;
            data.normalTextureIndex = material->normalTextureIndex;
            data.occlusionTextureIndex = material->occlusionTextureIndex;

            data.transformMatrix = node.transform.computeTransformMatrix();
            data.normalMatrix = node.transform.computeNormalMatrix();

            data.vertexAddress = node.mesh->vertexBuffer->getAddress();
            data.indexAddress = node.mesh->indexBuffer->getAddress();
        }

        instanceDataBuffer->copy(instanceData.data());
    }

    void createInstanceDataBuffer(const Context& context, const Scene& scene) {
        instanceData.clear();
        instanceData.resize(scene.nodes.size());
        instanceDataBuffer = context.createBuffer({
            .usage = rv::BufferUsage::Storage,
            .memory = rv::MemoryUsage::Device,
            .size = sizeof(InstanceData) * instanceData.size(),
            .debugName = "RenderWindow::instanceDataBuffer",
        });
    }

    void buildAccels(const Context& context, const Scene& scene) {
        std::unordered_map<std::string, int> meshIndices;
        bottomAccels.resize(scene.meshes.size());
        for (int i = 0; i < scene.meshes.size(); i++) {
            const Mesh& mesh = scene.meshes[i];
            meshIndices[mesh.name] = i;
            bottomAccels[i] = context.createBottomAccel({
                .vertexBuffer = mesh.vertexBuffer,
                .indexBuffer = mesh.indexBuffer,
                .vertexStride = sizeof(Vertex),
                .vertexCount = mesh.getVertexCount(),
                .triangleCount = mesh.getTriangleCount(),
            });
        }

        std::vector<AccelInstance> accelInstances;
        for (auto& node : scene.nodes) {
            if (node.mesh) {
                accelInstances.push_back({
                    .bottomAccel = bottomAccels[meshIndices[node.mesh->name]],
                    .transform = node.computeTransformMatrix(0.0),
                });
            }
        }
        topAccel = context.createTopAccel({.accelInstances = accelInstances});
    }

    rv::ImageHandle colorImage;
    vk::DescriptorSet imguiDescSet;

    rv::DescriptorSetHandle descSet;
    rv::RayTracingPipelineHandle pipeline;

    vk::UniqueCommandBuffer vkCommandBuffer;
    vk::UniqueFence fence;

    // Render data
    // NOTE: Vertex and Index buffers are accessed by buffer
    //       reference, so it is not necessary
    RenderPushConstants pushConstants;
    std::vector<InstanceData> instanceData;
    BufferHandle instanceDataBuffer;
    std::vector<BottomAccelHandle> bottomAccels;
    TopAccelHandle topAccel;

    bool running = false;

    IconManager iconManager;
};

class Editor : public App {
public:
    Editor()
        : App({
              .width = 2560,
              .height = 1440,
              .title = "Reactive Editor",
              .layers = {Layer::Validation},
              .extensions = {Extension::RayTracing},
          }) {}

    void onStart() override {
        // Add mesh
        scene.meshes.push_back(Mesh::createCubeMesh(context, {}));

        // Add material
        Material material;
        material.baseColor = glm::vec4{0.9, 0.3, 0.2, 1};
        material.name = "Standard 0";
        scene.materials.push_back(material);

        material.baseColor = glm::vec4{0.8, 0.7, 0.1, 1};
        material.name = "Standard 1";
        scene.materials.push_back(material);

        // Add node
        Node node;
        node.name = "Cube 0";
        node.mesh = &scene.meshes.back();
        node.material = &scene.materials[0];
        node.transform.translation = glm::vec3{-1.5, 0, 0};
        scene.nodes.push_back(node);

        node.name = "Cube 1";
        node.material = &scene.materials[1];
        node.transform.translation = glm::vec3{1.5, 0, 0};
        scene.nodes.push_back(node);

        camera = OrbitalCamera{this, 1920, 1080};
        camera.fovY = glm::radians(30.0f);
        camera.distance = 10.0f;

        assetWindow.init(context);
        viewportWindow.init(context, 1920, 1080);
        renderWindow.init(context, 1280, 720);
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
            renderWindow.show(scene, camera, frame);

            viewportWindow.drawContent(commandBuffer, scene, camera, frame);

            renderWindow.render(context, scene, camera, frame);

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
