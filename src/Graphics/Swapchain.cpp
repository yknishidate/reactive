#include "reactive/Graphics/Swapchain.hpp"
#include "reactive/Graphics/Fence.hpp"

namespace rv {
rv::Swapchain::Swapchain(const Context& context,
                         vk::SurfaceKHR surface,
                         uint32_t width,
                         uint32_t height,
                         vk::PresentModeKHR presentMode)
    : m_context{&context}, m_surface{surface}, m_presentMode{presentMode} {
    resize(width, height);
}

vk::SurfaceFormatKHR chooseSurfaceFormat(vk::PhysicalDevice physicalDevice,
                                         vk::SurfaceKHR surface) {
    auto availableFormats = physicalDevice.getSurfaceFormatsKHR(surface);
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Unorm) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

void Swapchain::resize(uint32_t width, uint32_t height) {
    m_swapchainImageViews.clear();
    m_swapchainImages.clear();
    m_swapchain.reset();

    vk::SurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(m_context->getPhysicalDevice(), m_surface);
    m_format = surfaceFormat.format;

    // Create swapchain
    uint32_t queueFamily = m_context->getQueueFamily();
    m_swapchain = m_context->getDevice().createSwapchainKHRUnique(
        vk::SwapchainCreateInfoKHR()
            .setSurface(m_surface)
            .setMinImageCount(m_minImageCount)
            .setImageFormat(m_format)
            .setImageColorSpace(surfaceFormat.colorSpace)
            .setImageExtent({width, height})
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment |
                           vk::ImageUsageFlagBits::eTransferDst)
            .setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
            .setPresentMode(m_presentMode)
            .setClipped(true)
            .setQueueFamilyIndices(queueFamily));

    // Get images
    m_swapchainImages = m_context->getDevice().getSwapchainImagesKHR(*m_swapchain);

    // Create image views
    for (auto& image : m_swapchainImages) {
        m_swapchainImageViews.push_back(m_context->getDevice().createImageViewUnique(
            vk::ImageViewCreateInfo()
                .setImage(image)
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(m_format)
                .setComponents({vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
                                vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA})
                .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})));
    }

    // Create command buffers and sync objects
    m_imageCount = static_cast<uint32_t>(m_swapchainImages.size());

    m_commandBuffers.resize(m_inflightCount);
    m_fences.resize(m_inflightCount);
    m_imageAcquiredSemaphores.resize(m_inflightCount);
    m_renderCompleteSemaphores.resize(m_inflightCount);
    for (uint32_t i = 0; i < m_inflightCount; i++) {
        m_commandBuffers[i] = m_context->allocateCommandBuffer();
        m_fences[i] = m_context->createFence({.signaled = true});
        m_imageAcquiredSemaphores[i] = m_context->getDevice().createSemaphoreUnique({});
        m_renderCompleteSemaphores[i] = m_context->getDevice().createSemaphoreUnique({});
    }
}

void Swapchain::waitNextFrame() {
    // Wait fence
    m_fences[m_inflightIndex]->wait();

    // Acquire next image
    auto acquireResult = m_context->getDevice().acquireNextImageKHR(
        *m_swapchain, UINT64_MAX, *m_imageAcquiredSemaphores[m_inflightIndex]);
    m_imageIndex = acquireResult.value;

    // Reset fence
    m_fences[m_inflightIndex]->reset();
}

void Swapchain::presentImage() {
    vk::PresentInfoKHR presentInfo;
    presentInfo.setWaitSemaphores(*m_renderCompleteSemaphores[m_inflightIndex]);
    presentInfo.setSwapchains(*m_swapchain);
    presentInfo.setImageIndices(m_imageIndex);
    if (m_context->getQueue().presentKHR(presentInfo) != vk::Result::eSuccess) {
        return;
    }
    m_inflightIndex = (m_inflightIndex + 1) % m_inflightCount;
}
}  // namespace rv
