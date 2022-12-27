#include "Swapchain.hpp"
#include "Context.hpp"
#include "Window/Window.hpp"
#include "Image.hpp"

Frame::Frame(vk::RenderPass renderPass, vk::ImageView attachment,
             uint32_t width, uint32_t height)
{
    framebuffer = Context::getDevice().createFramebufferUnique(
        vk::FramebufferCreateInfo()
        .setRenderPass(renderPass)
        .setAttachments(attachment)
        .setWidth(width)
        .setHeight(height)
        .setLayers(1)
    );
    commandBuffer = Context::allocateCommandBuffer();
    fence = Context::getDevice().createFenceUnique(
        vk::FenceCreateInfo()
        .setFlags(vk::FenceCreateFlagBits::eSignaled)
    );
}

Swapchain::Swapchain(vk::Device device, vk::SurfaceKHR surface, uint32_t queueFamily)
    : width{ Window::getWidth() }
    , height{ Window::getHeight() }
{
    swapchain = device.createSwapchainKHRUnique(
        vk::SwapchainCreateInfoKHR()
        .setSurface(surface)
        .setMinImageCount(minImageCount)
        .setImageFormat(vk::Format::eB8G8R8A8Unorm)
        .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
        .setImageExtent({ width, height })
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
        .setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
        .setPresentMode(vk::PresentModeKHR::eFifo)
        .setClipped(true)
        .setQueueFamilyIndices(queueFamily));

    // Get images
    swapchainImages = device.getSwapchainImagesKHR(*swapchain);

    // Create image views
    for (auto&& image : swapchainImages) {
        swapchainImageViews.push_back(device.createImageViewUnique(
            vk::ImageViewCreateInfo()
            .setImage(image)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(vk::Format::eB8G8R8A8Unorm)
            .setComponents({ vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA })
            .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 })
        ));
    }

    // Create render pass
    const auto attachment = vk::AttachmentDescription()
        .setFormat(vk::Format::eB8G8R8A8Unorm)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    const auto colorAttachment = vk::AttachmentReference()
        .setAttachment(0)
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    const auto subpass = vk::SubpassDescription()
        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachments(colorAttachment);

    const auto dependency = vk::SubpassDependency()
        .setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

    renderPass = device.createRenderPassUnique(
        vk::RenderPassCreateInfo()
        .setAttachments(attachment)
        .setSubpasses(subpass)
        .setDependencies(dependency));

    size_t imageCount = swapchainImages.size();
    frames = std::vector<Frame>(imageCount);
    imageAcquiredSemaphore = Context::getDevice().createSemaphoreUnique({});
    renderCompleteSemaphore = Context::getDevice().createSemaphoreUnique({});
    for (uint32_t i = 0; i < imageCount; i++) {
        frames[i] = Frame{ *renderPass, *swapchainImageViews[i], width, height };
    }
}

void Swapchain::beginRenderPass() const
{
    const auto renderArea = vk::Rect2D()
        .setExtent({ Window::getWidth(), Window::getHeight() });

    const auto clearColor = vk::ClearValue()
        .setColor(std::array{ 0.0f, 0.0f, 0.0f, 1.0f });

    const auto beginInfo = vk::RenderPassBeginInfo()
        .setRenderPass(*renderPass)
        .setClearValues(clearColor)
        .setFramebuffer(*frames[frameIndex].framebuffer)
        .setRenderArea(renderArea);

    frames[frameIndex].commandBuffer->beginRenderPass(beginInfo, vk::SubpassContents::eInline);
}

void Swapchain::endRenderPass() const
{
    frames[frameIndex].commandBuffer->endRenderPass();
}

void Swapchain::waitNextFrame(vk::Device device)
{
    try {
        frameIndex = device.acquireNextImageKHR(*swapchain, UINT64_MAX, *imageAcquiredSemaphore).value;
    } catch (const std::exception& exception) {
        swapchainRebuild = true;
        spdlog::error(exception.what());
        return;
    }
    frames[frameIndex].waitForFence(device);
}

vk::CommandBuffer Swapchain::beginCommandBuffer()
{
    return frames[frameIndex].beginCommandBuffer();
}

void Swapchain::submit(vk::Queue queue)
{
    frames[frameIndex].submitCommandBuffer(queue, *imageAcquiredSemaphore, *renderCompleteSemaphore);
}

void Swapchain::present(vk::Queue queue)
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

void Swapchain::copyToBackImage(vk::CommandBuffer commandBuffer, const Image& source)
{
    vk::ImageCopy copyRegion;
    copyRegion.setSrcSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });
    copyRegion.setDstSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });
    copyRegion.setExtent({ width, height, 1 });

    vk::Image backImage = getBackImage();
    vk::Image sourceImage = source.getImage();
    Image::setImageLayout(commandBuffer, sourceImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal);
    Image::setImageLayout(commandBuffer, backImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

    commandBuffer.copyImage(sourceImage, vk::ImageLayout::eTransferSrcOptimal,
                            backImage, vk::ImageLayout::eTransferDstOptimal, copyRegion);

    Image::setImageLayout(commandBuffer, sourceImage, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eGeneral);
    Image::setImageLayout(commandBuffer, backImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eColorAttachmentOptimal);
}
