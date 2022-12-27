#include "Context.hpp"
#include <regex>
#include <spdlog/spdlog.h>
#include "Window/Window.hpp"
#include "Graphics/Image.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

void Context::init()
{
    spdlog::info("Context::Init()");

    static const vk::DynamicLoader dl;
    const auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    // Create instance
    std::vector instanceExtensions = Window::getExtensions();
    std::vector layers{ "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
    const auto appInfo = vk::ApplicationInfo().setApiVersion(VK_API_VERSION_1_3);

    instance = vk::createInstanceUnique(vk::InstanceCreateInfo()
                                        .setPApplicationInfo(&appInfo)
                                        .setPEnabledExtensionNames(instanceExtensions)
                                        .setPEnabledLayerNames(layers));
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

    // Create debug messenger
    debugMessenger = instance->createDebugUtilsMessengerEXTUnique(
        vk::DebugUtilsMessengerCreateInfoEXT()
        .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
        .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
        .setPfnUserCallback(&debugCallback));

    surface = Window::createSurface(*instance);
    physicalDevice = instance->enumeratePhysicalDevices().front();

    // Find queue family
    std::vector properties = physicalDevice.getQueueFamilyProperties();
    for (uint32_t index = 0; index < properties.size(); index++) {
        auto supportGraphics = properties[index].queueFlags & vk::QueueFlagBits::eGraphics;
        auto supportCompute = properties[index].queueFlags & vk::QueueFlagBits::eCompute;
        if (surface) {
            auto supportPresent = physicalDevice.getSurfaceSupportKHR(index, *surface);
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
        vk::PhysicalDeviceDescriptorIndexingFeatures>
        createInfoChain{ deviceInfo,{ true }, { true }, { true }, descFeatures };

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

    swapchain = Swapchain{ *device, *surface, queueFamily };
}

std::vector<vk::UniqueCommandBuffer> Context::allocateCommandBuffers(uint32_t count)
{
    return device->allocateCommandBuffersUnique(
        vk::CommandBufferAllocateInfo()
        .setCommandPool(*commandPool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(count));
}

vk::UniqueCommandBuffer Context::allocateCommandBuffer()
{
    return std::move(allocateCommandBuffers(1).front());
}

void Context::oneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command)
{
    vk::UniqueCommandBuffer commandBuffer = allocateCommandBuffer();
    commandBuffer->begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    command(*commandBuffer);
    commandBuffer->end();
    queue.submit(vk::SubmitInfo().setCommandBuffers(*commandBuffer));
    queue.waitIdle();
}

uint32_t Context::findMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags memoryProp)
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

void Context::beginRenderPass()
{
    swapchain.beginRenderPass();
}

void Context::endRenderPass()
{
    swapchain.endRenderPass();
}

void Context::copyToBackImage(vk::CommandBuffer commandBuffer, const Image& source)
{
    swapchain.copyToBackImage(commandBuffer, source);
}

void Context::clearBackImage(vk::CommandBuffer commandBuffer, std::array<float, 4> color)
{
    Image::setImageLayout(commandBuffer, swapchain.getBackImage(),
                          vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    commandBuffer.clearColorImage(swapchain.getBackImage(),
                                  vk::ImageLayout::eTransferDstOptimal,
                                  vk::ClearColorValue{ color },
                                  vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
}
