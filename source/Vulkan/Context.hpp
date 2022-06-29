#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vector>
#include <vulkan/vulkan.hpp>
#include "Swapchain.hpp"

struct Context
{
    static void Init();
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
    static void CopyToBackImage(vk::CommandBuffer commandBuffer, const Image& source);

private:
    static inline vk::UniqueInstance instance;
    static inline vk::UniqueDebugUtilsMessengerEXT debugMessenger;
    static inline vk::PhysicalDevice physicalDevice;
    static inline vk::UniqueSurfaceKHR surface;
    static inline vk::UniqueDevice device;
    static inline uint32_t queueFamily = std::numeric_limits<uint32_t>::max();
    static inline vk::Queue queue;
    static inline vk::UniqueCommandPool commandPool;
    static inline vk::UniqueDescriptorPool descriptorPool;
    static inline Swapchain swapchain;
};
