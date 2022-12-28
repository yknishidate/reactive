#pragma once
#include <vulkan/vulkan.hpp>

class Image;

struct Swapchain
{
    Swapchain();

    void beginRenderPass() const;

    void endRenderPass() const;

    void waitNextFrame();

    vk::CommandBuffer beginCommandBuffer();

    void submit();

    void present();

    vk::RenderingAttachmentInfo getColorAttachmentInfo() const
    {
        vk::ClearValue clearColorValue;
        clearColorValue.setColor(vk::ClearColorValue{ std::array{0.0f, 0.0f, 0.0f, 1.0f} });

        vk::RenderingAttachmentInfo colorAttachment;
        colorAttachment.setImageView(*swapchainImageViews[frameIndex]);
        colorAttachment.setImageLayout(vk::ImageLayout::eAttachmentOptimal);
        colorAttachment.setClearValue(clearColorValue);
        colorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
        colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
        return colorAttachment;
    }

    vk::RenderingAttachmentInfo getDepthAttachmentInfo() const
    {
        vk::ClearValue clearDepthStencilValue;
        clearDepthStencilValue.setDepthStencil(vk::ClearDepthStencilValue{ 1.0f, 0 });

        vk::RenderingAttachmentInfo depthStencilAttachment;
        depthStencilAttachment.setImageView(*depthImageView);
        depthStencilAttachment.setImageLayout(vk::ImageLayout::eAttachmentOptimal);
        depthStencilAttachment.setClearValue(clearDepthStencilValue);
        depthStencilAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
        depthStencilAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
        return depthStencilAttachment;
    }

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

    void copyToBackImage(vk::CommandBuffer commandBuffer, const Image& source);

    auto getBackImage() const { return swapchainImages[frameIndex]; }
    auto getImageCount() const { return swapchainImages.size(); }
    auto getMinImageCount() const { return minImageCount; }
    auto getRenderPass() const { return *renderPass; }
    auto getCurrentCommandBuffer() const { return *commandBuffers[frameIndex]; }

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
