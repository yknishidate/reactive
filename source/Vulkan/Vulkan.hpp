#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vector>
#include <vulkan/vulkan.hpp>

struct Vulkan
{
    static void Init();
    static void Shutdown();

    static auto AllocateCommandBuffers(uint32_t count)->std::vector<vk::UniqueCommandBuffer>;
    static auto AllocateCommandBuffer()->vk::UniqueCommandBuffer;
    static void SubmitAndWait(vk::CommandBuffer commandBuffer);
    static void OneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command);
    static auto FindMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags memoryProp)->uint32_t;

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
};
