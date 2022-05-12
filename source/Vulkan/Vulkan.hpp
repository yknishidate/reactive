#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vector>
#include <vulkan/vulkan.hpp>

struct Frame
{
    vk::CommandBuffer commandBuffer{};
    vk::Fence fence{};
};

struct FrameSemaphores
{
    vk::Semaphore imageAcquiredSemaphore{};
    vk::Semaphore renderCompleteSemaphore{};
};

struct Vulkan
{
    static void Init();
    static void Shutdown();

    static auto AllocateCommandBuffers(uint32_t count)->std::vector<vk::UniqueCommandBuffer>;
    static auto AllocateCommandBuffer()->vk::UniqueCommandBuffer;
    static void SubmitAndWait(vk::CommandBuffer commandBuffer);
    static void OneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command);
    static auto FindMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags memoryProp)->uint32_t;

    static int GetCurrentImageIndex();
    static vk::CommandBuffer GetCurrentCommandBuffer();
    static vk::Image GetBackImage();
    static void StartFrame();
    static void WaitNextFrame();
    static void BeginCommandBuffer();
    static void Submit();
    static void Present();
    static void RebuildSwapchain();

    static inline vk::Instance instance;
    static inline vk::DebugUtilsMessengerEXT debugMessenger;
    static inline vk::PhysicalDevice physicalDevice;
    static inline vk::SurfaceKHR surface;
    static inline vk::Device device;
    static inline vk::SwapchainKHR swapchain;
    static inline std::vector<vk::Image> swapchainImages;
    static inline std::vector<vk::ImageView> swapchainImageViews;
    static inline uint32_t queueFamily = UINT32_MAX;
    static inline vk::Queue queue;
    static inline vk::CommandPool commandPool;
    static inline vk::DescriptorPool descriptorPool;

    static inline bool swapchainRebuild = false;
    static inline int minImageCount = 3;
    static inline uint32_t frameIndex = 0;
    static inline uint32_t imageCount = 0;
    static inline uint32_t semaphoreIndex = 0;
    static inline std::vector<Frame> frames{};
    static inline std::vector<FrameSemaphores> frameSemaphores{};
};
