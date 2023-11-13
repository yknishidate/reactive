#include "Graphics/Swapchain.hpp"
#include "Graphics/Fence.hpp"

namespace rv {
rv::Swapchain::Swapchain(const Context& context,
                         vk::SurfaceKHR surface,
                         uint32_t width,
                         uint32_t height,
                         vk::PresentModeKHR presentMode)
    : context{&context}, surface{surface}, presentMode{presentMode} {
    resize(width, height);
}

void Swapchain::resize(uint32_t width, uint32_t height) {
    swapchainImageViews.clear();
    swapchainImages.clear();
    swapchain.reset();

    // Create swapchain
    uint32_t queueFamily = context->getQueueFamily();
    swapchain = context->getDevice().createSwapchainKHRUnique(
        vk::SwapchainCreateInfoKHR()
            .setSurface(surface)
            .setMinImageCount(minImageCount)
            .setImageFormat(vk::Format::eB8G8R8A8Unorm)
            .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
            .setImageExtent({width, height})
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment |
                           vk::ImageUsageFlagBits::eTransferDst)
            .setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
            .setPresentMode(presentMode)
            .setClipped(true)
            .setQueueFamilyIndices(queueFamily));

    // Get images
    swapchainImages = context->getDevice().getSwapchainImagesKHR(*swapchain);

    // Create image views
    for (auto& image : swapchainImages) {
        swapchainImageViews.push_back(context->getDevice().createImageViewUnique(
            vk::ImageViewCreateInfo()
                .setImage(image)
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(vk::Format::eB8G8R8A8Unorm)
                .setComponents({vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
                                vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA})
                .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})));
    }

    // Create command buffers and sync objects
    imageCount = static_cast<uint32_t>(swapchainImages.size());

    commandBuffers.resize(inflightCount);
    fences.resize(inflightCount);
    imageAcquiredSemaphores.resize(inflightCount);
    renderCompleteSemaphores.resize(inflightCount);
    for (uint32_t i = 0; i < inflightCount; i++) {
        commandBuffers[i] = context->allocateCommandBuffer();
        fences[i] = context->createFence({.signaled = true});
        imageAcquiredSemaphores[i] = context->getDevice().createSemaphoreUnique({});
        renderCompleteSemaphores[i] = context->getDevice().createSemaphoreUnique({});
    }
}

vk::Result Swapchain::waitNextFrame() {
    // Wait fence
    vk::Result result = fences[inflightIndex]->wait();

    // Acquire next image
    auto acquireResult = context->getDevice().acquireNextImageKHR(
        *swapchain, UINT64_MAX, *imageAcquiredSemaphores[inflightIndex]);
    imageIndex = acquireResult.value;

    // Reset fence
    fences[inflightIndex]->reset();

    return result;
}

void Swapchain::presentImage() {
    vk::PresentInfoKHR presentInfo;
    presentInfo.setWaitSemaphores(*renderCompleteSemaphores[inflightIndex]);
    presentInfo.setSwapchains(*swapchain);
    presentInfo.setImageIndices(imageIndex);
    if (context->getQueue().presentKHR(presentInfo) != vk::Result::eSuccess) {
        return;
    }
    inflightIndex = (inflightIndex + 1) % inflightCount;
}
}  // namespace rv
