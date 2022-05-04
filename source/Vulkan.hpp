#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vector>
#include <vulkan/vulkan.hpp>

struct Vulkan
{
    static void Init();

    static auto AllocateCommandBuffers(uint32_t count)->std::vector<vk::UniqueCommandBuffer>;
    static auto AllocateCommandBuffer()->vk::UniqueCommandBuffer;
    static void SubmitAndWait(vk::CommandBuffer commandBuffer);
    static void OneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command);
    static auto FindMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags memoryProp)->uint32_t;

    static inline vk::Instance Instance;
    static inline vk::DebugUtilsMessengerEXT DebugMessenger;
    static inline vk::PhysicalDevice PhysicalDevice;
    static inline vk::SurfaceKHR Surface;
    static inline vk::Device Device;
    static inline vk::SwapchainKHR Swapchain;
    static inline std::vector<vk::Image> SwapchainImages;
    static inline std::vector<vk::ImageView> SwapchainImageViews;
    static inline uint32_t QueueFamily = UINT32_MAX;
    static inline vk::Queue Queue;
    static inline vk::CommandPool CommandPool;
    static inline vk::DescriptorPool DescriptorPool;
};
