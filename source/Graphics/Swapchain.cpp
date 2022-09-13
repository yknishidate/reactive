#include "Swapchain.hpp"
#include "Graphics.hpp"
#include "Window/Window.hpp"
#include "Helper.hpp"
#include "Image.hpp"

Frame::Frame(vk::RenderPass renderPass, vk::ImageView attachment,
             uint32_t width, uint32_t height)
{
    framebuffer = Graphics::GetDevice().createFramebufferUnique(
        vk::FramebufferCreateInfo()
        .setRenderPass(renderPass)
        .setAttachments(attachment)
        .setWidth(width)
        .setHeight(height)
        .setLayers(1)
    );
    commandBuffer = Graphics::AllocateCommandBuffer();
    fence = Graphics::GetDevice().createFenceUnique(
        vk::FenceCreateInfo()
        .setFlags(vk::FenceCreateFlagBits::eSignaled)
    );
}

Swapchain::Swapchain(vk::Device device, vk::SurfaceKHR surface, uint32_t queueFamily)
    : width{ Window::GetWidth() }
    , height{ Window::GetHeight() }
{
    swapchain = Helper::CreateSwapchain(device, surface, Window::GetWidth(), Window::GetHeight(),
                                        minImageCount, queueFamily);
    swapchainImages = device.getSwapchainImagesKHR(*swapchain);
    swapchainImageViews = Helper::CreateImageViews(device, swapchainImages);
    renderPass = Helper::CreateRenderPass(device);

    size_t imageCount = swapchainImages.size();
    frames = std::vector<Frame>(imageCount);
    imageAcquiredSemaphore = Graphics::GetDevice().createSemaphoreUnique({});
    renderCompleteSemaphore = Graphics::GetDevice().createSemaphoreUnique({});
    for (uint32_t i = 0; i < imageCount; i++) {
        frames[i] = Frame{ *renderPass, *swapchainImageViews[i], width, height };
    }
}

void Swapchain::BeginRenderPass()
{
    const auto renderArea = vk::Rect2D()
        .setExtent({ Window::GetWidth(), Window::GetHeight() });

    const auto beginInfo = vk::RenderPassBeginInfo()
        .setRenderPass(*renderPass)
        .setFramebuffer(*frames[frameIndex].framebuffer)
        .setRenderArea(renderArea);

    frames[frameIndex].commandBuffer->beginRenderPass(beginInfo, vk::SubpassContents::eInline);
}

void Swapchain::EndRenderPass()
{
    frames[frameIndex].commandBuffer->endRenderPass();
}

void Swapchain::WaitNextFrame(vk::Device device)
{
    try {
        frameIndex = device.acquireNextImageKHR(*swapchain, UINT64_MAX, *imageAcquiredSemaphore).value;
    } catch (const std::exception& exception) {
        swapchainRebuild = true;
        spdlog::error(exception.what());
        return;
    }
    frames[frameIndex].WaitForFence(device);
}

vk::CommandBuffer Swapchain::BeginCommandBuffer()
{
    return frames[frameIndex].BeginCommandBuffer();
}

void Swapchain::Submit(vk::Queue queue)
{
    frames[frameIndex].SubmitCommandBuffer(queue, *imageAcquiredSemaphore, *renderCompleteSemaphore);
}

void Swapchain::Present(vk::Queue queue)
{
    if (swapchainRebuild) {
        return;
    }

    const auto presentInfo = vk::PresentInfoKHR()
        .setWaitSemaphores(*renderCompleteSemaphore)
        .setSwapchains(*swapchain)
        .setImageIndices(frameIndex);

    if (queue.presentKHR(presentInfo) != vk::Result::eSuccess) {
        swapchainRebuild = true;
        return;
    }
    semaphoreIndex = (semaphoreIndex + 1) % swapchainImages.size();
}

void Swapchain::CopyToBackImage(vk::CommandBuffer commandBuffer, const Image& source)
{
    vk::ImageCopy copyRegion;
    copyRegion.setSrcSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });
    copyRegion.setDstSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });
    copyRegion.setExtent({ width, height, 1 });

    vk::Image backImage = GetBackImage();
    vk::Image sourceImage = source.GetImage();
    Image::SetImageLayout(commandBuffer, sourceImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal);
    Image::SetImageLayout(commandBuffer, backImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

    commandBuffer.copyImage(sourceImage, vk::ImageLayout::eTransferSrcOptimal,
                            backImage, vk::ImageLayout::eTransferDstOptimal, copyRegion);

    Image::SetImageLayout(commandBuffer, sourceImage, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eGeneral);
    Image::SetImageLayout(commandBuffer, backImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eColorAttachmentOptimal);
}
