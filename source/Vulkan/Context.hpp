#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vector>
#include <vulkan/vulkan.hpp>

struct FrameSemaphores
{
    FrameSemaphores();

    vk::UniqueSemaphore imageAcquiredSemaphore{};
    vk::UniqueSemaphore renderCompleteSemaphore{};
};

struct Frame
{
    Frame() = default;
    Frame(vk::RenderPass renderPass, vk::ImageView attachment);

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
        device.waitForFences(*fence, VK_TRUE, UINT64_MAX);
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

    void EndRenderPass()
    {
        frames[frameIndex].commandBuffer->endRenderPass();
    }

    void WaitNextFrame(vk::Device device)
    {
        const vk::Semaphore imageAcquiredSemaphore = *frameSemaphores[semaphoreIndex].imageAcquiredSemaphore;
        try {
            frameIndex = device.acquireNextImageKHR(*swapchain, UINT64_MAX, imageAcquiredSemaphore).value;
        } catch (const std::exception& exception) {
            swapchainRebuild = true;
            return;
        }
        frames[frameIndex].WaitForFence(device);
    }

    vk::CommandBuffer BeginCommandBuffer()
    {
        return frames[frameIndex].BeginCommandBuffer();
    }

    void Submit(vk::Queue queue)
    {
        frames[frameIndex].SubmitCommandBuffer(queue, frameSemaphores[semaphoreIndex]);
    }

    void Present(vk::Queue queue)
    {
        if (swapchainRebuild) {
            return;
        }

        try {
            queue.presentKHR(
                vk::PresentInfoKHR()
                .setWaitSemaphores(*frameSemaphores[semaphoreIndex].renderCompleteSemaphore)
                .setSwapchains(*swapchain)
                .setImageIndices(frameIndex)
            );
        } catch (const std::exception& exception) {
            std::cerr << "failed to present." << std::endl;
            swapchainRebuild = true;
            return;
        }
        semaphoreIndex = (semaphoreIndex + 1) % swapchainImages.size();
    }

    void RebuildSwapchain()
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

    auto GetBackImage() { return swapchainImages[frameIndex]; }
    auto GetImageCount() { return swapchainImages.size(); }
    auto GetMinImageCount() { return minImageCount; }
    auto GetRenderPass() { return *renderPass; }

    vk::UniqueSwapchainKHR swapchain;
    std::vector<vk::Image> swapchainImages;
    std::vector<vk::UniqueImageView> swapchainImageViews;

    bool swapchainRebuild = false;
    int minImageCount = 3;
    uint32_t frameIndex = 0;
    uint32_t imageCount = 0;
    uint32_t semaphoreIndex = 0;
    vk::UniqueRenderPass renderPass;
    std::vector<Frame> frames{};
    std::vector<FrameSemaphores> frameSemaphores{};
};

struct Context
{
    static void Init();
    static auto AllocateCommandBuffers(uint32_t count)->std::vector<vk::UniqueCommandBuffer>;
    static auto AllocateCommandBuffer()->vk::UniqueCommandBuffer;
    static void SubmitAndWait(vk::CommandBuffer commandBuffer);
    static void OneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command);
    static auto FindMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags memoryProp)->uint32_t;

    static auto GetInstance() { return *instance; }
    static auto GetDevice() { return *device; }
    static auto GetPhysicalDevice() { return physicalDevice; }
    static auto GetQueueFamily() { return queueFamily; }
    static auto GetQueue() { return queue; }
    static auto GetDescriptorPool() { return *descriptorPool; }
    static auto GetBackImage() { return swapchain.GetBackImage(); }
    static auto GetImageCount() { return swapchain.GetImageCount(); }
    static auto GetMinImageCount() { return swapchain.GetMinImageCount(); }
    static auto GetRenderPass() { return swapchain.GetRenderPass(); }

    static void BeginRenderPass();
    static void EndRenderPass();
    static void WaitNextFrame();
    static auto BeginCommandBuffer()->vk::CommandBuffer;
    static void Submit();
    static void Present();

private:
    static inline vk::UniqueInstance instance;
    static inline vk::UniqueDebugUtilsMessengerEXT debugMessenger;
    static inline vk::PhysicalDevice physicalDevice;
    static inline vk::UniqueSurfaceKHR surface;
    static inline vk::UniqueDevice device;
    static inline Swapchain swapchain;
    static inline uint32_t queueFamily = std::numeric_limits<uint32_t>::max();
    static inline vk::Queue queue;
    static inline vk::UniqueCommandPool commandPool;
    static inline vk::UniqueDescriptorPool descriptorPool;
};
