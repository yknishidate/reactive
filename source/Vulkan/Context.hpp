#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vector>
#include <vulkan/vulkan.hpp>

struct Frame
{
    vk::UniqueFramebuffer framebuffer{};
    vk::UniqueCommandBuffer commandBuffer{};
    vk::UniqueFence fence{};
};

struct FrameSemaphores
{
    vk::UniqueSemaphore imageAcquiredSemaphore{};
    vk::UniqueSemaphore renderCompleteSemaphore{};
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
    static auto GetBackImage() { return swapchainImages[frameIndex]; }
    static auto GetImageCount() { return swapchainImages.size(); }
    static auto GetMinImageCount() { return minImageCount; }
    static auto GetRenderPass() { return *renderPass; }

    static void BeginRenderPass();
    static void EndRenderPass();
    static void WaitNextFrame();
    static auto BeginCommandBuffer()->vk::CommandBuffer;
    static void Submit(vk::CommandBuffer commandBuffer);
    static void Present();
    static void RebuildSwapchain();

private:
    static inline vk::UniqueInstance instance;
    static inline vk::UniqueDebugUtilsMessengerEXT debugMessenger;
    static inline vk::PhysicalDevice physicalDevice;
    static inline vk::UniqueSurfaceKHR surface;
    static inline vk::UniqueDevice device;
    static inline vk::UniqueSwapchainKHR swapchain;
    static inline std::vector<vk::Image> swapchainImages;
    static inline std::vector<vk::UniqueImageView> swapchainImageViews;
    static inline uint32_t queueFamily = std::numeric_limits<uint32_t>::max();
    static inline vk::Queue queue;
    static inline vk::UniqueCommandPool commandPool;
    static inline vk::UniqueDescriptorPool descriptorPool;

    static inline bool swapchainRebuild = false;
    static inline int minImageCount = 3;
    static inline uint32_t frameIndex = 0;
    static inline uint32_t imageCount = 0;
    static inline uint32_t semaphoreIndex = 0;
    static inline vk::UniqueRenderPass renderPass;
    static inline std::vector<Frame> frames{};
    static inline std::vector<FrameSemaphores> frameSemaphores{};
};
