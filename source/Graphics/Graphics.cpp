#include "Graphics.hpp"
#include <regex>
#include <spdlog/spdlog.h>
#include "Window/Window.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
	void* pUserData)
{
	const std::string message{ pCallbackData->pMessage };
	const std::regex regex{ "The Vulkan spec states: " };
	std::smatch result;
	if (std::regex_search(message, result, regex)) {
		spdlog::error("{}\n", message.substr(0, result.position()));
	} else {
		spdlog::error("{}\n", message);
	}
	return VK_FALSE;
}

void Graphics::Init()
{
	spdlog::info("Graphics::Init()");

	std::vector instanceExtensions = Window::GetExtensions();
	std::vector layers{ "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };

	// Create instance
	static const vk::DynamicLoader dl;
	const auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	const auto appInfo = vk::ApplicationInfo()
		.setApiVersion(VK_API_VERSION_1_3);

	instance = vk::createInstanceUnique(
		vk::InstanceCreateInfo()
		.setPApplicationInfo(&appInfo)
		.setPEnabledExtensionNames(instanceExtensions)
		.setPEnabledLayerNames(layers));

	VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

	// Create debug messenger
	debugMessenger = instance->createDebugUtilsMessengerEXTUnique(
		vk::DebugUtilsMessengerCreateInfoEXT()
		.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
		.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
		.setPfnUserCallback(&DebugCallback));

	physicalDevice = instance->enumeratePhysicalDevices().front();

	surface = Window::CreateSurface(*instance);

	// Find queue family
	const std::vector properties = physicalDevice.getQueueFamilyProperties();
	for (uint32_t index = 0; index < properties.size(); index++) {
		const auto supportGraphics = properties[index].queueFlags & vk::QueueFlagBits::eGraphics;
		const auto supportCompute = properties[index].queueFlags & vk::QueueFlagBits::eCompute;
		const auto supportPresent = physicalDevice.getSurfaceSupportKHR(index, *surface);
		if (supportGraphics && supportCompute && supportPresent) {
			queueFamily = index;
		}
	}
	if (queueFamily == std::numeric_limits<uint32_t>::max()) {
		throw std::runtime_error("Failed to find queue family.");
	}

	// Create device
	constexpr float queuePriority = 1.0f;
	const auto queueInfo = vk::DeviceQueueCreateInfo()
		.setQueueFamilyIndex(queueFamily)
		.setQueueCount(1)
		.setPQueuePriorities(&queuePriority);

	const std::vector deviceExtensions{
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

	constexpr auto deviceFeatures = vk::PhysicalDeviceFeatures()
		.setShaderInt64(true)
		.setFragmentStoresAndAtomics(true)
		.setVertexPipelineStoresAndAtomics(true);

	constexpr auto descFeatures = vk::PhysicalDeviceDescriptorIndexingFeatures()
		.setRuntimeDescriptorArray(true);

	const auto deviceInfo = vk::DeviceCreateInfo()
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
	const std::vector<vk::DescriptorPoolSize> poolSizes{
		{ vk::DescriptorType::eSampler, 100 },
		{ vk::DescriptorType::eCombinedImageSampler, 100 },
		{ vk::DescriptorType::eSampledImage, 100 },
		{ vk::DescriptorType::eStorageImage, 100 },
		{ vk::DescriptorType::eUniformBuffer, 100 },
		{ vk::DescriptorType::eStorageBuffer, 100 },
		{ vk::DescriptorType::eInputAttachment, 100 }
	};

	descriptorPool = device->createDescriptorPoolUnique(
		vk::DescriptorPoolCreateInfo()
		.setPoolSizes(poolSizes)
		.setMaxSets(100)
		.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
	);

	swapchain = Swapchain{ *device, *surface, queueFamily };
}

vk::UniqueCommandBuffer Graphics::AllocateCommandBuffer()
{
	return std::move(device->allocateCommandBuffersUnique(
		vk::CommandBufferAllocateInfo()
		.setCommandPool(*commandPool)
		.setLevel(vk::CommandBufferLevel::ePrimary)
		.setCommandBufferCount(1)).front());
}

void Graphics::SubmitAndWait(vk::CommandBuffer commandBuffer)
{
	queue.submit(
		vk::SubmitInfo()
		.setCommandBuffers(commandBuffer));
	queue.waitIdle();
}

void Graphics::OneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command)
{
	vk::UniqueCommandBuffer commandBuffer = AllocateCommandBuffer();

	commandBuffer->begin(
		vk::CommandBufferBeginInfo()
		.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
	command(*commandBuffer);
	commandBuffer->end();

	SubmitAndWait(*commandBuffer);
}

uint32_t Graphics::FindMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags memoryProp)
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

void Graphics::BeginRenderPass()
{
	swapchain.BeginRenderPass();
}

void Graphics::EndRenderPass()
{
	swapchain.EndRenderPass();
}

void Graphics::CopyToBackImage(vk::CommandBuffer commandBuffer, const Image& source)
{
	swapchain.CopyToBackImage(commandBuffer, source);
}
