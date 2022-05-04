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
    // Init
    spdlog::set_pattern("[%^%l%$] %v");
    spdlog::info("Engine::Init()");
    Window::Init(1920, 1080, "../asset/Vulkan.png");
    Vulkan::Init();
    Window::SetupUI();

    // Create resources
    inputImage.Init(Window::GetWidth(), Window::GetHeight(), vk::Format::eB8G8R8A8Unorm);
    outputImage.Init(Window::GetWidth(), Window::GetHeight(), vk::Format::eB8G8R8A8Unorm);
    mesh = std::make_shared<Mesh>("../asset/viking_room/viking_room.obj");
    texture.Init("../asset/viking_room/viking_room.png");

    object.Init(mesh);
    topAccel.InitAsTop(object.GetMesh().GetAccel());

    BufferAddress address;
    address.vertices = mesh->GetVertexBufferAddress();
    address.indices = mesh->GetIndexBufferAddress();
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
    pushConstants.invProj = glm::inverse(camera.GetProj());
    pushConstants.invView = glm::inverse(camera.GetView());
    pushConstants.frame = 0;
}

void Engine::Shutdown()
{
    Vulkan::Device.waitIdle();
    Window::Shutdown();
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
        pushConstants.invProj = glm::inverse(camera.GetProj());
        pushConstants.invView = glm::inverse(camera.GetView());
        pushConstants.frame++;
        if (camera.CheckDirtyAndClean()) {
            pushConstants.frame = 0;
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
}
