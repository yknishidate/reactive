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

    uint32_t getCurrentInFlightIndex() const { return inflightIndex; }

    rv::CommandBufferHandle getCurrentCommandBuffer() const {
        return commandBuffers[inflightIndex];
    }

    vk::Image getCurrentImage() const { return swapchainImages[imageIndex]; }

    vk::ImageView getCurrentImageView() const { return *swapchainImageViews[imageIndex]; }

    vk::Semaphore getCurrentImageAcquiredSemaphore() const {
        return *imageAcquiredSemaphores[inflightIndex];
    }

    vk::Semaphore getCurrentRenderCompleteSemaphore() const {
        return *renderCompleteSemaphores[inflightIndex];
    }

    FenceHandle getCurrentFence() const { return fences[inflightIndex]; }

    uint32_t getMinImageCount() const { return minImageCount; }

    uint32_t getImageCount() const { return imageCount; }

    uint32_t getInFlightCount() const { return inflightCount; }

private:
    const Context* context = nullptr;

    vk::UniqueSwapchainKHR swapchain;
    std::vector<vk::Image> swapchainImages;
    std::vector<vk::UniqueImageView> swapchainImageViews;

    vk::SurfaceKHR surface;
    vk::PresentModeKHR presentMode;

    uint32_t minImageCount = 3;
    uint32_t imageCount = 0;
    uint32_t imageIndex = 0;

    uint32_t inflightCount = 3;
    uint32_t inflightIndex = 0;

    std::vector<vk::UniqueSemaphore> imageAcquiredSemaphores;
    std::vector<vk::UniqueSemaphore> renderCompleteSemaphores;
    std::vector<CommandBufferHandle> commandBuffers{};
    std::vector<FenceHandle> fences{};
};
}  // namespace rv
