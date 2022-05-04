#include <functional>
#include "Engine.hpp"
#include "Vulkan.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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


    std::vector<Vertex> vertices{
        { { 1.0,  0.0, 0.0} },
        { { 0.0, -1.0, 0.0} },
        { {-1.0,  0.0, 0.0} } };
    std::vector<uint32_t> indices{ 0, 1, 2 };

    mesh.Init(vertices, indices);
    topAccel.InitAsTop(mesh.GetAccel());

    // Create pipelines
    pipeline.Init("../shader/simple.comp");
    pipeline.UpdateDescSet("outImage", renderImage.GetView(), renderImage.GetSampler());

    rtPipeline.Init("../shader/hello_raytracing/hello_raytracing.rgen",
                    "../shader/hello_raytracing/hello_raytracing.rmiss",
                    "../shader/hello_raytracing/hello_raytracing.rchit", sizeof(PushConstants));
    rtPipeline.UpdateDescSet("renderImage", renderImage.GetView(), renderImage.GetSampler());
    rtPipeline.UpdateDescSet("topLevelAS", topAccel.GetAccel());

    // Create push constants
    pushConstants.invProj = glm::inverse(glm::perspective(glm::radians(45.0f), float(Window::GetWidth()) / Window::GetHeight(), 0.01f, 10000.0f));
    pushConstants.invView = glm::inverse(glm::lookAt(glm::vec3{ 0, 0, 5 }, glm::vec3{ 0, 0, 3 }, { 0.0f, 1.0f, 0.0f }));
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
            rtPipeline.Run(commandBuffer, width, height, &pushConstants);
            CopyToBackImage(commandBuffer, width, height, renderImage.GetImage(), Window::GetBackImage());

            Window::RenderUI();
            Window::Submit();
            Window::Present();
        }
    }
}
