#pragma once
#include <vulkan/vulkan.hpp>
#include "CommandBuffer.hpp"

class Swapchain
{
public:
    Swapchain();

    void waitNextFrame();

    CommandBuffer beginCommandBuffer();

    void present();

    void rebuildSwapchain()
    {
        //int width, height;
        //glfwGetFramebufferSize(window, &width, &height);
        //if (width > 0 && height > 0) {
        //    ImGui_ImplVulkan_SetMinImageCount(minImageCount);
        //    ImGui_ImplVulkanH_CreateOrResizeWindow(instance, physicalDevice, device, &windowData, Context::queueFamily, nullptr, width, height, minImageCount);
        //    windowData.FrameIndex = 0;
        //    swapchainRebuild = false;
        //}
    }

    auto getBackImage() const { return swapchainImages[frameIndex]; }
    auto getImageCount() const { return swapchainImages.size(); }
    auto getMinImageCount() const { return minImageCount; }
    auto getRenderPass() const { return *renderPass; }
    auto getCurrentCommandBuffer() const { return *commandBuffers[frameIndex]; }

private:
    friend class CommandBuffer;
    void beginRenderPass() const;
    void endRenderPass() const;
    void submit();
    void copyToBackImage(vk::CommandBuffer commandBuffer, const Image& source);
    void clearBackImage(vk::CommandBuffer commandBuffer, std::array<float, 4> color);

    vk::UniqueSwapchainKHR swapchain;
    std::vector<vk::Image> swapchainImages;
    std::vector<vk::UniqueImageView> swapchainImageViews;
    uint32_t width = 0;
    uint32_t height = 0;

    vk::UniqueImage depthImage;
    vk::UniqueImageView depthImageView;
    vk::UniqueDeviceMemory depthImageMemory;

    bool swapchainRebuild = false;
    int minImageCount = 3;
    uint32_t frameIndex = 0;
    uint32_t imageCount = 0;
    uint32_t semaphoreIndex = 0;

    // TODO: move to GUI
    vk::UniqueRenderPass renderPass;
    vk::UniqueSemaphore imageAcquiredSemaphore;
    vk::UniqueSemaphore renderCompleteSemaphore;
    std::vector<vk::UniqueCommandBuffer> commandBuffers{};
    std::vector<vk::UniqueFramebuffer> framebuffers{};
    std::vector<vk::UniqueFence> fences{};
};
