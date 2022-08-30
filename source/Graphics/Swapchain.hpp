#pragma once
#include <vulkan/vulkan.hpp>

class Image;

struct FrameSemaphores
{
    FrameSemaphores();

    vk::UniqueSemaphore imageAcquiredSemaphore{};
    vk::UniqueSemaphore renderCompleteSemaphore{};
};

struct Frame
{
    Frame() = default;
    Frame(vk::RenderPass renderPass, vk::ImageView attachment,
          uint32_t width, uint32_t height);

    vk::CommandBuffer BeginCommandBuffer()
    {
        commandBuffer->begin(
            vk::CommandBufferBeginInfo()
            .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
        );
        return *commandBuffer;
    }

    void SubmitCommandBuffer(vk::Queue queue, const FrameSemaphores& semaphores)
    {
        commandBuffer->end();

        vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

        const auto submitInfo = vk::SubmitInfo()
            .setWaitDstStageMask(waitStage)
            .setCommandBuffers(*commandBuffer)
            .setWaitSemaphores(*semaphores.imageAcquiredSemaphore)
            .setSignalSemaphores(*semaphores.renderCompleteSemaphore);

        queue.submit(submitInfo, *fence);
    }

    void WaitForFence(vk::Device device)
    {
        vk::Result result = device.waitForFences(*fence, VK_TRUE, UINT64_MAX);
        if (result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to wait for fence");
        }
        device.resetFences(*fence);
    }

    vk::UniqueFramebuffer framebuffer{};
    vk::UniqueCommandBuffer commandBuffer{};
    vk::UniqueFence fence{};
};

struct Swapchain
{
    Swapchain() = default;
    Swapchain(vk::Device device, vk::SurfaceKHR surface, uint32_t queueFamily);

    void BeginRenderPass();

    void EndRenderPass();

    void WaitNextFrame(vk::Device device);

    vk::CommandBuffer BeginCommandBuffer();

    void Submit(vk::Queue queue);

    void Present(vk::Queue queue);

    void RebuildSwapchain()
    {
        //int width, height;
        //glfwGetFramebufferSize(window, &width, &height);
        //if (width > 0 && height > 0) {
        //    ImGui_ImplVulkan_SetMinImageCount(minImageCount);
        //    ImGui_ImplVulkanH_CreateOrResizeWindow(instance, physicalDevice, device, &windowData, Graphics::queueFamily, nullptr, width, height, minImageCount);
        //    windowData.FrameIndex = 0;
        //    swapchainRebuild = false;
        //}
    }

    void CopyToBackImage(vk::CommandBuffer commandBuffer, const Image& source);

    auto GetBackImage() const { return swapchainImages[frameIndex]; }
    auto GetImageCount() const { return swapchainImages.size(); }
    auto GetMinImageCount() const { return minImageCount; }
    auto GetRenderPass() const { return *renderPass; }
    auto GetCurrentCommandBuffer() const { return *frames[frameIndex].commandBuffer; }

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
    vk::UniqueRenderPass renderPass;
    std::vector<Frame> frames{};
    std::vector<FrameSemaphores> frameSemaphores{};
};
