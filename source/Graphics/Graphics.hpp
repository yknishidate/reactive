#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include "Swapchain.hpp"
#include "Instance.hpp"

struct Graphics
{
	static void Init();
	static auto AllocateCommandBuffer() -> vk::UniqueCommandBuffer;
	static void SubmitAndWait(vk::CommandBuffer commandBuffer);
	static void OneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command);
	static auto FindMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags memoryProp) -> uint32_t;

	static auto GetInstance() { return instance.GetInstance(); }
	static auto GetDevice() { return *device; }
	static auto GetPhysicalDevice() { return physicalDevice; }
	static auto GetQueueFamily() { return queueFamily; }
	static auto GetQueue() { return queue; }
	static auto GetDescriptorPool() { return *descriptorPool; }
	static auto GetImageCount() { return swapchain.GetImageCount(); }
	static auto GetMinImageCount() { return swapchain.GetMinImageCount(); }
	static auto GetRenderPass() { return swapchain.GetRenderPass(); }
	static const auto& GetSwapchain() { return swapchain; }

	static void BeginRenderPass();
	static void EndRenderPass();
	static void CopyToBackImage(vk::CommandBuffer commandBuffer, const Image& source);
	static void WaitNextFrame() { swapchain.WaitNextFrame(*device); }
	static auto BeginCommandBuffer() -> vk::CommandBuffer { return swapchain.BeginCommandBuffer(); }
	static void Submit() { swapchain.Submit(queue); }
	static void Present() { swapchain.Present(queue); }

private:
	static inline Instance instance;
	static inline vk::PhysicalDevice physicalDevice;
	static inline vk::UniqueSurfaceKHR surface;
	static inline vk::UniqueDevice device;
	static inline uint32_t queueFamily = std::numeric_limits<uint32_t>::max();
	static inline vk::Queue queue;
	static inline vk::UniqueCommandPool commandPool;
	static inline vk::UniqueDescriptorPool descriptorPool;
	static inline Swapchain swapchain;
};
