#include <functional>
#include "Engine.hpp"
#include "Vulkan.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glm/glm.hpp>

namespace
{
    void CopyToBackImage(vk::CommandBuffer commandBuffer, int width, int height,
                         vk::Image renderImage, vk::Image backImage)
    {
        vk::ImageCopy copyRegion;
        copyRegion.setSrcSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });
        copyRegion.setDstSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });
        copyRegion.setExtent({ uint32_t(width), uint32_t(height), 1 });

        SetImageLayout(commandBuffer, renderImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal);
        SetImageLayout(commandBuffer, backImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        CopyImage(commandBuffer, renderImage, backImage, copyRegion);
        SetImageLayout(commandBuffer, renderImage, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eGeneral);
        SetImageLayout(commandBuffer, backImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eColorAttachmentOptimal);
    }
}

struct PushConstants
{
    glm::mat4 invView;
    glm::mat4 invProj;
    int frame = 0;
};

void Engine::Init()
{
    // Init
    spdlog::set_pattern("[%^%l%$] %v");
    spdlog::info("Engine::Init()");
    Window::Init(1920, 1080);
    std::vector layers{ "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
    Vulkan::Init(Window::GetExtensions(), layers);
    Window::SetupUI();

    // Create resources
    renderImage.Init(Window::GetWidth(), Window::GetHeight(), vk::Format::eB8G8R8A8Unorm);

    pipeline.Init("../shader/simple.comp");
    pipeline.UpdateDescSet("outImage", renderImage.GetView(), renderImage.GetSampler());

    rtPipeline.Init("../shader/hello_raytracing/hello_raytracing.rgen",
                    "../shader/hello_raytracing/hello_raytracing.rmiss",
                    "../shader/hello_raytracing/hello_raytracing.rchit", sizeof(PushConstants));
    rtPipeline.UpdateDescSet("renderImage", renderImage.GetView(), renderImage.GetSampler());

    using Vertex = glm::vec3;
    std::vector<Vertex> vertices{
        { 1.0,  0.0, 0.0},
        { 0.0, -1.0, 0.0},
        {-1.0,  0.0, 0.0} };
    std::vector<uint32_t> indices{ 0, 1, 2 };

    vk::BufferUsageFlags usage{
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress
    };

    Buffer vertexBuffer;
    Buffer indexBuffer;
    vertexBuffer.InitOnHost(sizeof(Vertex) * vertices.size(), usage);
    indexBuffer.InitOnHost(sizeof(uint32_t) * indices.size(), usage);
    vertexBuffer.Copy(vertices.data());
    indexBuffer.Copy(indices.data());

    Accel bottomAccel;
    Accel topAccel;
    bottomAccel.InitAsBottom(vertexBuffer, indexBuffer, sizeof(Vertex), vertices.size(), indices.size() / 3);
    topAccel.InitAsTop(bottomAccel);
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
        Window::StartFrame();

        if (!Window::IsMinimized()) {
            Window::WaitNextFrame();
            Window::BeginCommandBuffer();

            int width = Window::GetWidth();
            int height = Window::GetHeight();
            vk::CommandBuffer commandBuffer = Window::GetCurrentCommandBuffer();
            //pipeline.Run(commandBuffer, width, height);
            rtPipeline.Run(commandBuffer, width, height);
            CopyToBackImage(commandBuffer, width, height, renderImage.GetImage(), Window::GetBackImage());

            Window::RenderUI();
            Window::Submit();
            Window::Present();
        }
    }
}
