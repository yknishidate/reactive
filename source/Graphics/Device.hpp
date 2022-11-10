#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <functional>
#include "Swapchain.hpp"

struct Device
{
	friend class Instance;

	Device() = default;

	vk::UniqueCommandBuffer AllocateCommandBuffer()
	{
		return std::move(device->allocateCommandBuffersUnique(
			vk::CommandBufferAllocateInfo()
			.setCommandPool(*commandPool)
			.setLevel(vk::CommandBufferLevel::ePrimary)
			.setCommandBufferCount(1)).front());
	}

	void SubmitAndWait(vk::CommandBuffer commandBuffer) const
	{
		queue.submit(vk::SubmitInfo()
			.setCommandBuffers(commandBuffer));
		queue.waitIdle();
	}

	void OneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command)
	{
		vk::UniqueCommandBuffer commandBuffer = AllocateCommandBuffer();

		commandBuffer->begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
		command(*commandBuffer);
		commandBuffer->end();

		SubmitAndWait(*commandBuffer);
	}

	uint32_t FindMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags memoryProp) const
	{
		const vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();
		for (uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index) {
			auto propertyFlags = memoryProperties.memoryTypes[index].propertyFlags;
			bool match = (propertyFlags & memoryProp) == memoryProp;
			if (requirements.memoryTypeBits & (1 << index) && match) {
				return index;
			}
		}
		throw std::runtime_error("Failed to find memory type index.");
	}

	// TODO: remove
	auto GetDevice() const { return *device; }
	auto GetPhysicalDevice() const { return physicalDevice; }
	auto GetQueueFamily() const { return queueFamily; }
	auto GetQueue() const { return queue; }
	auto GetDescriptorPool() const { return *descriptorPool; }

	Swapchain CreateSwapchain(vk::SurfaceKHR surface)
	{
		return { *device, surface, queueFamily };
	}

private:
	Device(vk::Instance instance, vk::SurfaceKHR surface)
	{
		physicalDevice = instance.enumeratePhysicalDevices().front();

		// Find queue family
		std::vector properties = physicalDevice.getQueueFamilyProperties();
		for (uint32_t index = 0; index < properties.size(); index++) {
			auto supportGraphics = properties[index].queueFlags & vk::QueueFlagBits::eGraphics;
			auto supportCompute = properties[index].queueFlags & vk::QueueFlagBits::eCompute;
			if (surface) {
				auto supportPresent = physicalDevice.getSurfaceSupportKHR(index, surface);
				if (supportGraphics && supportCompute && supportPresent) {
					queueFamily = index;
				}
			} else {
				if (supportGraphics && supportCompute) {
					queueFamily = index;
				}
			}
		}
		if (queueFamily == std::numeric_limits<uint32_t>::max()) {
			throw std::runtime_error("Failed to find queue family.");
		}

		// Create device
		float queuePriority = 1.0f;
		auto queueInfo = vk::DeviceQueueCreateInfo()
			.setQueueFamilyIndex(queueFamily)
			.setQueueCount(1)
			.setPQueuePriorities(&queuePriority);

		std::vector deviceExtensions{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
			VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
			VK_KHR_MAINTENANCE3_EXTENSION_NAME,
			VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
			VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
			VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
		};

		auto deviceFeatures = vk::PhysicalDeviceFeatures()
			.setShaderInt64(true)
			.setFragmentStoresAndAtomics(true)
			.setVertexPipelineStoresAndAtomics(true);

		auto descFeatures = vk::PhysicalDeviceDescriptorIndexingFeatures()
			.setRuntimeDescriptorArray(true);

		auto deviceInfo = vk::DeviceCreateInfo()
			.setQueueCreateInfos(queueInfo)
			.setPEnabledExtensionNames(deviceExtensions)
			.setPEnabledFeatures(&deviceFeatures);

		vk::StructureChain<vk::DeviceCreateInfo,
			vk::PhysicalDeviceBufferDeviceAddressFeatures,
			vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
			vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
			vk::PhysicalDeviceDynamicRenderingFeatures,
			vk::PhysicalDeviceDescriptorIndexingFeatures>
			createInfoChain{ deviceInfo,{ true }, { true }, { true }, { true }, descFeatures };

		device = physicalDevice.createDeviceUnique(createInfoChain.get<vk::DeviceCreateInfo>());

		queue = device->getQueue(queueFamily, 0);

		// Create command pool
		commandPool = device->createCommandPoolUnique(
			vk::CommandPoolCreateInfo()
			.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
			.setQueueFamilyIndex(queueFamily));

		// Create descriptor pool
		std::vector<vk::DescriptorPoolSize> poolSizes{
		   { vk::DescriptorType::eSampler, 100 },
		   { vk::DescriptorType::eCombinedImageSampler, 100 },
		   { vk::DescriptorType::eSampledImage, 100 },
		   { vk::DescriptorType::eStorageImage, 100 },
		   { vk::DescriptorType::eUniformBuffer, 100 },
		   { vk::DescriptorType::eStorageBuffer, 100 },
		   { vk::DescriptorType::eInputAttachment, 100 },
		   { vk::DescriptorType::eAccelerationStructureKHR, 100 } };

		descriptorPool = device->createDescriptorPoolUnique(
			vk::DescriptorPoolCreateInfo()
			.setPoolSizes(poolSizes)
			.setMaxSets(100)
			.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet));
	}

	vk::UniqueDevice device;
	vk::PhysicalDevice physicalDevice;
	uint32_t queueFamily = std::numeric_limits<uint32_t>::max();
	vk::Queue queue;
	vk::UniqueCommandPool commandPool;
	vk::UniqueDescriptorPool descriptorPool;
};
