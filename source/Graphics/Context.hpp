#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include "Swapchain.hpp"
#include "Instance.hpp"

struct Context
{
    static void Init();

    static auto AllocateCommandBuffer() -> vk::UniqueCommandBuffer
    {
        return device.AllocateCommandBuffer();
    }

    static void SubmitAndWait(vk::CommandBuffer commandBuffer)
    {
        return device.SubmitAndWait(commandBuffer);
    }

    static void OneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command)
    {
        return device.OneTimeSubmit(command);
    }

    static auto FindMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags memoryProp) -> uint32_t
    {
        return device.FindMemoryTypeIndex(requirements, memoryProp);
    }

    static auto GetInstance() { return instance.GetInstance(); }
    static auto GetDevice() { return device.GetDevice(); }
    static auto GetPhysicalDevice() { return device.GetPhysicalDevice(); }
    static auto GetQueueFamily() { return device.GetQueueFamily(); }
    static auto GetQueue() { return device.GetQueue(); }
    static auto GetDescriptorPool() { return device.GetDescriptorPool(); }
    static auto GetImageCount() { return swapchain.GetImageCount(); }
    static auto GetMinImageCount() { return swapchain.GetMinImageCount(); }
    static auto GetRenderPass() { return swapchain.GetRenderPass(); }
    static const auto& GetSwapchain() { return swapchain; }

    static void BeginRenderPass();
    static void EndRenderPass();
    static void CopyToBackImage(vk::CommandBuffer commandBuffer, const Image& source);
    static void WaitNextFrame() { swapchain.WaitNextFrame(device.GetDevice()); }
    static auto BeginCommandBuffer() -> vk::CommandBuffer { return swapchain.BeginCommandBuffer(); }
    static void Submit() { swapchain.Submit(device.GetQueue()); }
    static void Present() { swapchain.Present(device.GetQueue()); }

private:
    static inline Instance instance;
    static inline vk::UniqueSurfaceKHR surface;
    static inline Device device;
    static inline Swapchain swapchain;
};
