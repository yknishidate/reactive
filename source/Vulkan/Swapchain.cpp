#include "Swapchain.hpp"
#include "Context.hpp"
#include "Window.hpp"
#include "Helper.hpp"

Frame::Frame(vk::RenderPass renderPass, vk::ImageView attachment,
             uint32_t width, uint32_t height)
{
    framebuffer = Context::GetDevice().createFramebufferUnique(
        vk::FramebufferCreateInfo()
        .setRenderPass(renderPass)
        .setAttachments(attachment)
        .setWidth(width)
        .setHeight(height)
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
    frameSemaphores = std::vector<FrameSemaphores>(imageCount);
    for (uint32_t i = 0; i < imageCount; i++) {
        frames[i] = Frame{ *renderPass, *swapchainImageViews[i], width, height };
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

void Swapchain::EndRenderPass()
{
    frames[frameIndex].commandBuffer->endRenderPass();
}

void Swapchain::WaitNextFrame(vk::Device device)
{
    const vk::Semaphore imageAcquiredSemaphore = *frameSemaphores[semaphoreIndex].imageAcquiredSemaphore;
    try {
        frameIndex = device.acquireNextImageKHR(*swapchain, UINT64_MAX, imageAcquiredSemaphore).value;
    } catch (const std::exception& exception) {
        swapchainRebuild = true;
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
    frames[frameIndex].SubmitCommandBuffer(queue, frameSemaphores[semaphoreIndex]);
}

void Swapchain::Present(vk::Queue queue)
{
    if (swapchainRebuild) {
        return;
    }

    try {
        queue.presentKHR(
            vk::PresentInfoKHR()
            .setWaitSemaphores(*frameSemaphores[semaphoreIndex].renderCompleteSemaphore)
            .setSwapchains(*swapchain)
            .setImageIndices(frameIndex)
        );
    } catch (const std::exception& exception) {
        std::cerr << "failed to present." << std::endl;
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

    Image::CopyImage(commandBuffer, sourceImage, backImage, copyRegion);

    Image::SetImageLayout(commandBuffer, sourceImage, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eGeneral);
    Image::SetImageLayout(commandBuffer, backImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eColorAttachmentOptimal);
}
