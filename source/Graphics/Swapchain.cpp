#include "Swapchain.hpp"
#include "Context.hpp"
#include "Graphics/Image.hpp"
#include "Window/Window.hpp"

Swapchain::Swapchain() : width{Window::getWidth()}, height{Window::getHeight()} {
    uint32_t queueFamily = Context::getQueueFamily();
    swapchain = Context::getDevice().createSwapchainKHRUnique(
        vk::SwapchainCreateInfoKHR()
            .setSurface(Context::getSurface())
            .setMinImageCount(minImageCount)
            .setImageFormat(vk::Format::eB8G8R8A8Unorm)
            .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
            .setImageExtent({width, height})
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment |
                           vk::ImageUsageFlagBits::eTransferDst)
            .setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
            .setPresentMode(vk::PresentModeKHR::eFifo)
            .setClipped(true)
            .setQueueFamilyIndices(queueFamily));

    // Get images
    swapchainImages = Context::getDevice().getSwapchainImagesKHR(*swapchain);

    // Create image views
    for (auto& image : swapchainImages) {
        swapchainImageViews.push_back(Context::getDevice().createImageViewUnique(
            vk::ImageViewCreateInfo()
                .setImage(image)
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(vk::Format::eB8G8R8A8Unorm)
                .setComponents({vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
                                vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA})
                .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})));
    }

    // Create depth image
    depthImage = {width, height, vk::Format::eD32Sfloat, ImageUsage::DepthStencilAttachment};

    // Create render pass
    vk::AttachmentDescription colorAttachment;
    colorAttachment.setFormat(vk::Format::eB8G8R8A8Unorm);
    colorAttachment.setSamples(vk::SampleCountFlagBits::e1);
    colorAttachment.setLoadOp(vk::AttachmentLoadOp::eDontCare);
    colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
    colorAttachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    colorAttachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    colorAttachment.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentDescription depthAttachment = depthImage.createAttachmentDesc();
    std::array attachmentDescs = {colorAttachment, depthAttachment};

    vk::AttachmentReference colorAttachmentRef;
    colorAttachmentRef.setAttachment(0);
    colorAttachmentRef.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentReference depthAttachmentRef = depthImage.createAttachmentRef(1);
    renderPass = RenderPass{width, height, attachmentDescs, colorAttachmentRef, depthAttachmentRef};

    size_t imageCount = swapchainImages.size();
    commandBuffers = Context::allocateCommandBuffers(imageCount);
    framebuffers.resize(imageCount);
    fences.resize(imageCount);
    imageAcquiredSemaphore = Context::getDevice().createSemaphoreUnique({});
    renderCompleteSemaphore = Context::getDevice().createSemaphoreUnique({});
    for (uint32_t i = 0; i < imageCount; i++) {
        std::array attachments{*swapchainImageViews[i], depthImage.getView()};
        framebuffers[i] = Framebuffer{width, height, renderPass, attachments};
        fences[i] = Context::getDevice().createFenceUnique(
            vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled));
    }
}

void Swapchain::setBackImageLayout(vk::CommandBuffer commandBuffer, vk::ImageLayout layout) {
    Image::setImageLayout(commandBuffer, swapchainImages[frameIndex], layout);
}

void Swapchain::beginRenderPass() const {
    renderPass.beginRenderPass(*commandBuffers[frameIndex],
                               framebuffers[frameIndex].getFramebuffer());
}

void Swapchain::endRenderPass() const {
    renderPass.endRenderPass(*commandBuffers[frameIndex]);
}

void Swapchain::waitNextFrame() {
    try {
        frameIndex = Context::getDevice()
                         .acquireNextImageKHR(*swapchain, UINT64_MAX, *imageAcquiredSemaphore)
                         .value;
    } catch (const std::exception& exception) {
        swapchainRebuild = true;
        spdlog::error(exception.what());
        return;
    }
    vk::Result result =
        Context::getDevice().waitForFences(*fences[frameIndex], VK_TRUE, UINT64_MAX);
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for fence");
    }
    Context::getDevice().resetFences(*fences[frameIndex]);
}

CommandBuffer Swapchain::beginCommandBuffer() {
    commandBuffers[frameIndex]->begin(
        vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    return {this, *commandBuffers[frameIndex]};
}

void Swapchain::submit() {
    commandBuffers[frameIndex]->end();
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submitInfo;
    submitInfo.setWaitDstStageMask(waitStage);
    submitInfo.setCommandBuffers(*commandBuffers[frameIndex]);
    submitInfo.setWaitSemaphores(*imageAcquiredSemaphore);
    submitInfo.setSignalSemaphores(*renderCompleteSemaphore);
    Context::getQueue().submit(submitInfo, *fences[frameIndex]);
}

void Swapchain::present() {
    if (swapchainRebuild) {
        return;
    }
    vk::PresentInfoKHR presentInfo;
    presentInfo.setWaitSemaphores(*renderCompleteSemaphore);
    presentInfo.setSwapchains(*swapchain);
    presentInfo.setImageIndices(frameIndex);
    if (Context::getQueue().presentKHR(presentInfo) != vk::Result::eSuccess) {
        swapchainRebuild = true;
        return;
    }
    semaphoreIndex = (semaphoreIndex + 1) % swapchainImages.size();
}

void Swapchain::copyToBackImage(vk::CommandBuffer commandBuffer, Image& source) {
    vk::ImageCopy copyRegion;
    copyRegion.setSrcSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});
    copyRegion.setDstSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});
    copyRegion.setExtent({width, height, 1});

    vk::Image backImage = getBackImage();
    source.setImageLayout(commandBuffer, vk::ImageLayout::eTransferSrcOptimal);
    Image::setImageLayout(commandBuffer, backImage, vk::ImageLayout::eTransferDstOptimal);
    commandBuffer.copyImage(source.getImage(), vk::ImageLayout::eTransferSrcOptimal, backImage,
                            vk::ImageLayout::eTransferDstOptimal, copyRegion);
    source.setImageLayout(commandBuffer, vk::ImageLayout::eGeneral);
    Image::setImageLayout(commandBuffer, backImage, vk::ImageLayout::ePresentSrcKHR);
}

void Swapchain::clearBackImage(vk::CommandBuffer commandBuffer, std::array<float, 4> color) {
    Image::setImageLayout(commandBuffer, getBackImage(), vk::ImageLayout::eTransferDstOptimal);
    commandBuffer.clearColorImage(
        getBackImage(), vk::ImageLayout::eTransferDstOptimal, vk::ClearColorValue{color},
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
}
