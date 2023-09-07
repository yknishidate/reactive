#pragma once
#include <nfd.h>
#include <stb_image_write.h>

#include <future>

#include "AttributeWindow.hpp"
#include "IconManager.hpp"
#include "Scene.hpp"
#include "reactive/Graphics/Accel.hpp"
#include "reactive/Scene/Mesh.hpp"
#include "reactive/common.hpp"
#include "shader/share.h"

class RenderWindow {
public:
    void init(const rv::Context& context, const Scene& scene, uint32_t width, uint32_t height) {
        this->context = &context;
        this->scene = &scene;
        createImages(width, height);

        vkCommandBuffer = context.allocateCommandBuffer();

        vk::FenceCreateInfo fenceInfo;
        fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
        fence = context.getDevice().createFenceUnique(fenceInfo);
        iconManager.addIcon(context, "render_ipr", ASSET_DIR + "icons/render_ipr.png");
        iconManager.addIcon(context, "render_save", ASSET_DIR + "icons/render_save.png");
    }

    void createPipeline() {
        std::vector<rv::ShaderHandle> shaders(3);
        shaders[0] = context->createShader({
            .code = rv::Compiler::compileToSPV(SHADER_DIR + "base.rgen"),
            .stage = vk::ShaderStageFlagBits::eRaygenKHR,
        });

        shaders[1] = context->createShader({
            .code = rv::Compiler::compileToSPV(SHADER_DIR + "base.rmiss"),
            .stage = vk::ShaderStageFlagBits::eMissKHR,
        });

        shaders[2] = context->createShader({
            .code = rv::Compiler::compileToSPV(SHADER_DIR + "base.rchit"),
            .stage = vk::ShaderStageFlagBits::eClosestHitKHR,
        });

        descSet = context->createDescriptorSet({
            .shaders = shaders,
            .images = {{"accumImage", accumImage}, {"outputImage", colorImage}},
            .accels = {{"topLevelAS", topAccel}},
        });

        pipeline = context->createRayTracingPipeline({
            .rgenShaders = shaders[0],
            .missShaders = shaders[1],
            .chitShaders = shaders[2],
            .descSetLayout = descSet->getLayout(),
            .pushSize = sizeof(RenderPushConstants),
            .maxRayRecursionDepth = 16,
        });
    }

    void createImages(uint32_t width, uint32_t height) {
        accumImage = context->createImage({
            .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage |
                     vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc |
                     vk::ImageUsageFlagBits::eColorAttachment,
            .extent = {width, height, 1},
            .format = vk::Format::eR32G32B32A32Sfloat,
            .layout = vk::ImageLayout::eGeneral,
            .debugName = "RenderWindow::accumImage",
        });

        colorImage = context->createImage({
            .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage |
                     vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc |
                     vk::ImageUsageFlagBits::eColorAttachment,
            .extent = {width, height, 1},
            .format = vk::Format::eR8G8B8A8Unorm,
            .layout = vk::ImageLayout::eGeneral,
            .debugName = "RenderWindow::colorImage",
        });

        imageSavingBuffer = context->createBuffer({
            .usage = rv::BufferUsage::Staging,
            .memory = rv::MemoryUsage::Host,
            .size = sizeof(char) * width * height * 4,
            .debugName = "RenderWindow::imageSavingBuffer",
        });

        // Create desc set
        ImGui_ImplVulkan_RemoveTexture(imguiDescSet);
        imguiDescSet = ImGui_ImplVulkan_AddTexture(colorImage->getSampler(), colorImage->getView(),
                                                   VK_IMAGE_LAYOUT_GENERAL);
    }

    void showRunIcon(float thumbnailSize) {
        ImVec4 bgColor = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
        if (running || iconManager.isHover(thumbnailSize)) {
            bgColor = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
        }
        iconManager.showIcon("render_ipr", "", thumbnailSize, bgColor, [&]() {
            if (running) {
                running = false;
                spdlog::info("[UI] Stop IPR");
            } else {
                running = true;
                spdlog::info("[UI] Start IPR");
            }
        });
    }

    void flipY(uint8_t* pixels, uint32_t width, uint32_t height) const {
        uint32_t rowPitch = width * 4;
        auto row = static_cast<uint8_t*>(malloc(rowPitch));
        for (uint32_t i = 0; i < (height / 2); i++) {
            uint8_t* row0 = pixels + i * rowPitch;
            uint8_t* row1 = pixels + (height - i - 1) * rowPitch;
            memcpy(row, row0, rowPitch);
            memcpy(row0, row1, rowPitch);
            memcpy(row1, row, rowPitch);
        }
        free(row);
    }

    void saveImage(const char* savePath) {
        vk::Extent3D imageExtent = colorImage->getExtent();
        context->oneTimeSubmit([&](rv::CommandBuffer commandBuffer) {
            commandBuffer.transitionLayout(colorImage, vk::ImageLayout::eTransferSrcOptimal);
            commandBuffer.copyImageToBuffer(colorImage, imageSavingBuffer);
            commandBuffer.transitionLayout(colorImage, vk::ImageLayout::eGeneral);
        });

        writeTask = std::async(std::launch::async, [=]() {
            auto* pixels = static_cast<uint8_t*>(imageSavingBuffer->map());
            flipY(pixels, imageExtent.width, imageExtent.height);

            std::filesystem::path filepath = savePath;
            std::filesystem::path extension = filepath.extension();
            if (extension == ".png") {
                stbi_write_png(savePath, imageExtent.width, imageExtent.height, 4, pixels, 90);
            } else if (extension == ".jpg" || extension == ".jpeg") {
                stbi_write_jpg(savePath, imageExtent.width, imageExtent.height, 4, pixels, 90);
            } else {
                RV_ASSERT(false, "Unknown file extension: {}", extension.string());
            }
        });
    }

    void openSaveDialog() {
        nfdchar_t* savePath = nullptr;
        nfdresult_t result = NFD_SaveDialog("png,jpg", nullptr, &savePath);
        if (result == NFD_OKAY) {
            saveImage(savePath);
            free(savePath);
            spdlog::info("[UI] Save image");
        }
    }

    void showSaveIcon(float thumbnailSize) {
        ImVec4 bgColor = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
        if (iconManager.isHover(thumbnailSize)) {
            bgColor = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
        }
        iconManager.showIcon("render_save", "", thumbnailSize, bgColor,
                             [&]() { openSaveDialog(); });
    }

    void showToolBar() {
        float thumbnailSize = 50.0f;
        int columnCount = 2;
        float padding = 5.0f;
        float panelWidth = columnCount * (thumbnailSize + padding);
        ImGui::BeginChild("Toolbar", ImVec2(panelWidth, 60), false,
                          ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
        ImGui::Columns(columnCount, nullptr, true);

        showRunIcon(thumbnailSize);
        showSaveIcon(thumbnailSize);

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::EndChild();
    }

    void show(const rv::Camera& camera, int frame) {
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

    void updatePushConstants(rv::Camera& camera, int message) {
        vk::Extent3D imageExtent = colorImage->getExtent();
        float tmpAspect = camera.aspect;
        float tmpFovY = camera.fovY;
        camera.aspect = static_cast<float>(imageExtent.width) / imageExtent.height;
        camera.fovY /= camera.aspect;
        pushConstants.invView = camera.getInvView();
        pushConstants.invProj = camera.getInvProj();
        pushConstants.instanceDataAddress = instanceDataBuffer->getAddress();
        if (message == Message::None) {
            // Accumulate
            pushConstants.frame++;
        } else {
            // Reset
            pushConstants.frame = 0;
        }
        camera.aspect = tmpAspect;
        camera.fovY = tmpFovY;
    }

    void findDomeLight() {
        for (int i = 0; i < scene->nodes.size(); i++) {
            const auto& node = scene->nodes[i];
            if (node.type == Node::DomeLightNode) {
                pushConstants.domeLightIndex = i;
            }
        }
    }

    void loadScene() {
        createInstanceDataBuffer();
        updateInstanceDataBuffer();
        buildAccels();
        createPipeline();
        findDomeLight();
    }

    void render(rv::Camera& camera, int frame, int message) {
        if (!running) {
            return;
        }

        if (!topAccel) {
            loadScene();
        }

        // TODO: move to new function
        // Ignore the request if rendering is in progress.
        vk::Result fenceStatus = context->getDevice().getFenceStatus(*fence);
        if (fenceStatus == vk::Result::eNotReady) {
            spdlog::info("RenderWindow::render(): Rendering is in progress.");
            return;
        }
        context->getDevice().resetFences(*fence);

        if (message & Message::MaterialChanged || message & Message::TransformChanged) {
            spdlog::info("Update instance data");
            updateInstanceDataBuffer();
        }

        updatePushConstants(camera, message);

        vk::CommandBufferBeginInfo beginInfo;
        vkCommandBuffer->begin(beginInfo);

        // Update top level as
        if (message & Message::TransformChanged) {
            spdlog::info("Update top level as");
            updateTopAccel(*vkCommandBuffer, frame);
        }

        vk::Extent3D imageExtent = colorImage->getExtent();
        rv::CommandBuffer commandBuffer{context, *vkCommandBuffer};
        commandBuffer.bindDescriptorSet(descSet, pipeline);
        commandBuffer.bindPipeline(pipeline);
        commandBuffer.clearColorImage(colorImage, {1.0, 0.0, 1.0, 1.0});
        commandBuffer.transitionLayout(colorImage, vk::ImageLayout::eGeneral);
        if (message != Message::None) {
            commandBuffer.clearColorImage(accumImage, {0.0, 0.0, 0.0, 1.0});
            commandBuffer.transitionLayout(accumImage, vk::ImageLayout::eGeneral);
        }
        commandBuffer.pushConstants(pipeline, &pushConstants);
        commandBuffer.traceRays(pipeline, imageExtent.width, imageExtent.height, imageExtent.depth);

        vkCommandBuffer->end();
        context->submit(*vkCommandBuffer, *fence);
    }

    void updateInstanceDataBuffer() {
        for (int i = 0; i < scene->nodes.size(); i++) {
            const Node& node = scene->nodes[i];
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

            if (node.mesh) {
                data.vertexAddress = node.mesh->vertexBuffer->getAddress();
                data.indexAddress = node.mesh->indexBuffer->getAddress();
            }
        }

        instanceDataBuffer->copy(instanceData.data());
    }

    void createInstanceDataBuffer() {
        instanceData.clear();
        instanceData.resize(scene->nodes.size());
        instanceDataBuffer = context->createBuffer({
            .usage = rv::BufferUsage::Storage,
            .memory = rv::MemoryUsage::Device,
            .size = sizeof(InstanceData) * instanceData.size(),
            .debugName = "RenderWindow::instanceDataBuffer",
        });
    }

    void buildAccels() {
        std::unordered_map<std::string, int> meshIndices;
        bottomAccels.resize(scene->meshes.size());
        for (int i = 0; i < scene->meshes.size(); i++) {
            const rv::Mesh& mesh = scene->meshes[i];
            meshIndices[mesh.name] = i;
            bottomAccels[i] = context->createBottomAccel({
                .vertexBuffer = mesh.vertexBuffer,
                .indexBuffer = mesh.indexBuffer,
                .vertexStride = sizeof(rv::Vertex),
                .vertexCount = mesh.getVertexCount(),
                .triangleCount = mesh.getTriangleCount(),
            });
        }

        std::vector<rv::AccelInstance> accelInstances;
        for (auto& node : scene->nodes) {
            if (node.mesh) {
                accelInstances.push_back({
                    .bottomAccel = bottomAccels[meshIndices[node.mesh->name]],
                    .transform = node.computeTransformMatrix(0.0),
                });
            }
        }
        topAccel = context->createTopAccel({.accelInstances = accelInstances});
    }

    void updateTopAccel(vk::CommandBuffer commandBuffer, int frame) {
        // TODO: I want to remove this loop.
        std::unordered_map<std::string, int> meshIndices;
        bottomAccels.resize(scene->meshes.size());
        for (int i = 0; i < scene->meshes.size(); i++) {
            const rv::Mesh& mesh = scene->meshes[i];
            meshIndices[mesh.name] = i;
        }

        std::vector<rv::AccelInstance> accelInstances;
        for (auto& node : scene->nodes) {
            if (node.mesh) {
                accelInstances.push_back({
                    .bottomAccel = bottomAccels[meshIndices[node.mesh->name]],
                    .transform = node.computeTransformMatrix(frame),
                });
            }
        }
        topAccel->update(commandBuffer, accelInstances);
    }

    const rv::Context* context;
    const Scene* scene;
    rv::ImageHandle accumImage;
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
    rv::BufferHandle instanceDataBuffer;
    std::vector<rv::BottomAccelHandle> bottomAccels;
    rv::TopAccelHandle topAccel;

    bool running = false;

    std::future<void> writeTask;
    IconManager iconManager;
    rv::BufferHandle imageSavingBuffer;
};
