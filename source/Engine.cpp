#include <functional>
#include "Engine.hpp"
#include "Vulkan.hpp"
#include "Input.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

struct BufferAddress
{
    vk::DeviceAddress vertices;
    vk::DeviceAddress indices;
    vk::DeviceAddress objects;
};

namespace
{
    void CopyImages(vk::CommandBuffer commandBuffer, int width, int height,
                    vk::Image inputImage, vk::Image outputImage, vk::Image backImage)
    {
        vk::ImageCopy copyRegion;
        copyRegion.setSrcSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });
        copyRegion.setDstSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });
        copyRegion.setExtent({ uint32_t(width), uint32_t(height), 1 });

        Image::SetImageLayout(commandBuffer, outputImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal);
        Image::SetImageLayout(commandBuffer, inputImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferDstOptimal);
        Image::SetImageLayout(commandBuffer, backImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

        Image::CopyImage(commandBuffer, outputImage, inputImage, copyRegion);
        Image::CopyImage(commandBuffer, outputImage, backImage, copyRegion);

        Image::SetImageLayout(commandBuffer, outputImage, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eGeneral);
        Image::SetImageLayout(commandBuffer, inputImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral);
        Image::SetImageLayout(commandBuffer, backImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eColorAttachmentOptimal);
    }
}

void Engine::Init()
{
    spdlog::info("Engine::Init()");

    // Create resources
    inputImage.Init(Window::GetWidth(), Window::GetHeight(), vk::Format::eB8G8R8A8Unorm);
    outputImage.Init(Window::GetWidth(), Window::GetHeight(), vk::Format::eB8G8R8A8Unorm);
    //mesh = std::make_shared<Mesh>("../asset/viking_room/viking_room.obj");
    mesh = std::make_shared<Mesh>("../asset/CornellBox.obj");
    texture.Init("../asset/viking_room/viking_room.png");

    objects.resize(1);
    objects[0].Init(mesh);
    topAccel.InitAsTop(objects);

    // Create object data
    for (auto&& object : objects) {
        objectData.push_back({ object.GetTransform().GetMatrix(), object.GetTransform().GetNormalMatrix(), -1 });
    }
    objectBuffer.InitOnHost(sizeof(ObjectData) * objectData.size(),
                            vk::BufferUsageFlagBits::eStorageBuffer |
                            vk::BufferUsageFlagBits::eShaderDeviceAddress);
    objectBuffer.Copy(objectData.data());

    BufferAddress address;
    address.vertices = mesh->GetVertexBufferAddress();
    address.indices = mesh->GetIndexBufferAddress();
    address.objects = objectBuffer.GetAddress();
    addressBuffer.InitOnHost(sizeof(BufferAddress), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress);
    addressBuffer.Copy(&address);

    // Create pipeline
    rtPipeline.Init("../shader/pathtracing/pathtracing.rgen",
                    "../shader/pathtracing/pathtracing.rmiss",
                    "../shader/pathtracing/pathtracing.rchit", sizeof(PushConstants));
    rtPipeline.UpdateDescSet("inputImage", inputImage.GetView(), inputImage.GetSampler());
    rtPipeline.UpdateDescSet("outputImage", outputImage.GetView(), outputImage.GetSampler());
    rtPipeline.UpdateDescSet("topLevelAS", topAccel.GetAccel());
    rtPipeline.UpdateDescSet("samplers", texture.GetView(), texture.GetSampler());
    rtPipeline.UpdateDescSet("Addresses", addressBuffer.GetBuffer(), addressBuffer.GetSize());

    // Create push constants
    camera.Init(Window::GetWidth(), Window::GetHeight());
    pushConstants.InvProj = glm::inverse(camera.GetProj());
    pushConstants.InvView = glm::inverse(camera.GetView());
    pushConstants.Frame = 0;
}

void Engine::Run()
{
    spdlog::info("Engine::Run()");
    while (!Window::ShouldClose()) {
        Window::PollEvents();
        Input::Update();
        Window::StartFrame();

        // Update push constants
        camera.ProcessInput();
        pushConstants.InvProj = glm::inverse(camera.GetProj());
        pushConstants.InvView = glm::inverse(camera.GetView());
        pushConstants.Frame++;
        if (camera.CheckDirtyAndClean()) {
            pushConstants.Frame = 0;
        }

        // Render
        if (!Window::IsMinimized()) {
            Window::WaitNextFrame();
            Window::BeginCommandBuffer();

            int width = Window::GetWidth();
            int height = Window::GetHeight();
            vk::CommandBuffer commandBuffer = Window::GetCurrentCommandBuffer();
            rtPipeline.Run(commandBuffer, width, height, &pushConstants);
            CopyImages(commandBuffer, width, height, inputImage.GetImage(), outputImage.GetImage(), Window::GetBackImage());

            Window::RenderUI();
            Window::Submit();
            Window::Present();
        }
    }
    Vulkan::Device.waitIdle();
}
