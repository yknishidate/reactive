#include "Swapchain.hpp"
#include "Context.hpp"
#include "Window.hpp"
#include "Helper.hpp"

Frame::Frame(vk::RenderPass renderPass, vk::ImageView attachment)
{
    framebuffer = Context::GetDevice().createFramebufferUnique(
        vk::FramebufferCreateInfo()
        .setRenderPass(renderPass)
        .setAttachments(attachment)
        .setWidth(Window::GetWidth())
        .setHeight(Window::GetHeight())
        .setLayers(1)
    );
    commandBuffer = Context::AllocateCommandBuffer();
    fence = Context::GetDevice().createFenceUnique(
        vk::FenceCreateInfo()
        .setFlags(vk::FenceCreateFlagBits::eSignaled)
    );
}

FrameSemaphores::FrameSemaphores()
{
    imageAcquiredSemaphore = Context::GetDevice().createSemaphoreUnique({});
    renderCompleteSemaphore = Context::GetDevice().createSemaphoreUnique({});
}

Swapchain::Swapchain(vk::Device device, vk::SurfaceKHR surface, uint32_t queueFamily)
{
    swapchain = Helper::CreateSwapchain(device, surface, Window::GetWidth(), Window::GetHeight(),
                                        minImageCount, queueFamily);
    swapchainImages = device.getSwapchainImagesKHR(*swapchain);
    swapchainImageViews = Helper::CreateImageViews(device, swapchainImages);
    renderPass = Helper::CreateRenderPass(device);

    size_t imageCount = swapchainImages.size();
    frames = std::vector<Frame>(imageCount);
    frameSemaphores = std::vector<FrameSemaphores>(imageCount);
    for (uint32_t i = 0; i < imageCount; i++) {
        frames[i] = Frame{ *renderPass, *swapchainImageViews[i] };
    }
}

void Swapchain::BeginRenderPass()
{
    const auto renderArea = vk::Rect2D()
        .setExtent({ static_cast<uint32_t>(Window::GetWidth()), static_cast<uint32_t>(Window::GetHeight()) });

    const auto beginInfo = vk::RenderPassBeginInfo()
        .setRenderPass(*renderPass)
        .setFramebuffer(*frames[frameIndex].framebuffer)
        .setRenderArea(renderArea);

    frames[frameIndex].commandBuffer->beginRenderPass(beginInfo, vk::SubpassContents::eInline);
}
