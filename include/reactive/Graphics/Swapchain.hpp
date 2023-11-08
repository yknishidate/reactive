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

    rv::CommandBufferHandle getCurrentCommandBuffer() const { return commandBuffers[frameIndex]; }

    vk::Image getCurrentImage() const { return swapchainImages[frameIndex]; }

    vk::ImageView getCurrentImageView() const { return *swapchainImageViews[frameIndex]; }

    vk::Semaphore getCurrentImageAcquiredSemaphore() const {
        return *imageAcquiredSemaphores[semaphoreIndex];
    }

    vk::Semaphore getCurrentRenderCompleteSemaphore() const {
        return *renderCompleteSemaphores[semaphoreIndex];
    }

    FenceHandle getCurrentFence() const { return fences[frameIndex]; }

    uint32_t getMinImageCount() const { return minImageCount; }

    uint32_t getImageCount() const { return imageCount; }

private:
    const Context* context = nullptr;

    vk::UniqueSwapchainKHR swapchain;
    std::vector<vk::Image> swapchainImages;
    std::vector<vk::UniqueImageView> swapchainImageViews;

    vk::SurfaceKHR surface;
    vk::PresentModeKHR presentMode;

    uint32_t minImageCount = 3;
    uint32_t frameIndex = 0;
    uint32_t imageCount = 0;
    uint32_t semaphoreIndex = 0;

    std::vector<vk::UniqueSemaphore> imageAcquiredSemaphores;
    std::vector<vk::UniqueSemaphore> renderCompleteSemaphores;
    std::vector<CommandBufferHandle> commandBuffers{};
    std::vector<FenceHandle> fences{};
};
}  // namespace rv
