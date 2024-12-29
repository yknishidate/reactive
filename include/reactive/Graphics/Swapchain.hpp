#pragma once
#include "Context.hpp"

namespace rv {
class Swapchain {
public:
    Swapchain(const Context& context,
              vk::SurfaceKHR surface,
              uint32_t width,
              uint32_t height,
              vk::PresentModeKHR presentMode);

    void resize(uint32_t width, uint32_t height);

    void waitNextFrame();

    void presentImage();

    uint32_t getCurrentInFlightIndex() const { return m_inflightIndex; }

    rv::CommandBufferHandle getCurrentCommandBuffer() const {
        return m_commandBuffers[m_inflightIndex];
    }

    vk::Image getCurrentImage() const { return m_swapchainImages[m_imageIndex]; }

    vk::ImageView getCurrentImageView() const { return *m_swapchainImageViews[m_imageIndex]; }

    vk::Semaphore getCurrentImageAcquiredSemaphore() const {
        return *m_imageAcquiredSemaphores[m_inflightIndex];
    }

    vk::Semaphore getCurrentRenderCompleteSemaphore() const {
        return *m_renderCompleteSemaphores[m_inflightIndex];
    }

    FenceHandle getCurrentFence() const { return m_fences[m_inflightIndex]; }

    uint32_t getMinImageCount() const { return m_minImageCount; }

    uint32_t getImageCount() const { return m_imageCount; }

    uint32_t getInFlightCount() const { return m_inflightCount; }

    vk::Format getFormat() const { return m_format; }

private:
    const Context* m_context = nullptr;

    vk::UniqueSwapchainKHR m_swapchain;
    std::vector<vk::Image> m_swapchainImages;
    std::vector<vk::UniqueImageView> m_swapchainImageViews;

    vk::SurfaceKHR m_surface;
    vk::PresentModeKHR m_presentMode;
    vk::Format m_format = vk::Format::eB8G8R8A8Unorm;

    uint32_t m_minImageCount = 3;
    uint32_t m_imageCount = 0;
    uint32_t m_imageIndex = 0;

    uint32_t m_inflightCount = 3;
    uint32_t m_inflightIndex = 0;

    std::vector<vk::UniqueSemaphore> m_imageAcquiredSemaphores;
    std::vector<vk::UniqueSemaphore> m_renderCompleteSemaphores;
    std::vector<CommandBufferHandle> m_commandBuffers{};
    std::vector<FenceHandle> m_fences{};
};
}  // namespace rv
