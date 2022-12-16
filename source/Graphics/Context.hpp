#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <spdlog/spdlog.h>
#include <regex>
#include <functional>
#include "Swapchain.hpp"
#include "Device.hpp"

struct Context
{
    static void Init();

    static void CreateInstance(const std::vector<const char*>& extensions, const std::vector<const char*>& layers) {
        static const vk::DynamicLoader dl;
        const auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        // Create instance
        const auto appInfo = vk::ApplicationInfo().setApiVersion(VK_API_VERSION_1_3);

        instance = vk::createInstanceUnique(vk::InstanceCreateInfo()
                                            .setPApplicationInfo(&appInfo)
                                            .setPEnabledExtensionNames(extensions)
                                            .setPEnabledLayerNames(layers));
        VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

        // Create debug messenger
        debugMessenger = instance->createDebugUtilsMessengerEXTUnique(
            vk::DebugUtilsMessengerCreateInfoEXT()
            .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
            .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
            .setPfnUserCallback(&DebugCallback));
    }

    //static auto AllocateCommandBuffer() -> vk::UniqueCommandBuffer
    //{
    //    return device.AllocateCommandBuffer();
    //}

    static vk::UniqueCommandBuffer AllocateCommandBuffer()
    {
        return std::move(device->allocateCommandBuffersUnique(
            vk::CommandBufferAllocateInfo()
            .setCommandPool(*commandPool)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount(1)).front());
    }

    //static void SubmitAndWait(vk::CommandBuffer commandBuffer)
    //{
    //    return device.SubmitAndWait(commandBuffer);
    //}

    static void SubmitAndWait(vk::CommandBuffer commandBuffer)
    {
        queue.submit(vk::SubmitInfo()
                     .setCommandBuffers(commandBuffer));
        queue.waitIdle();
    }

    //static void OneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command)
    //{
    //    return device.OneTimeSubmit(command);
    //}

    static void OneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command)
    {
        vk::UniqueCommandBuffer commandBuffer = AllocateCommandBuffer();
        commandBuffer->begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        command(*commandBuffer);
        commandBuffer->end();
        SubmitAndWait(*commandBuffer);
    }

    //static auto FindMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags memoryProp) -> uint32_t
    //{
    //    return device.FindMemoryTypeIndex(requirements, memoryProp);
    //}

    static uint32_t FindMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags memoryProp)
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

    static auto GetInstance() { return *instance; }
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
    static VKAPI_ATTR VkBool32 VKAPI_CALL
        DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                      VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                      VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                      void* pUserData) {
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

    static void CreateDevice()
    {
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

    //static inline Instance instance;
    static inline vk::UniqueInstance instance;
    static inline vk::UniqueDebugUtilsMessengerEXT debugMessenger;

    static inline vk::UniqueSurfaceKHR surface;

    //static inline Device device;
    static inline vk::UniqueDevice device;
    static inline vk::PhysicalDevice physicalDevice;
    static inline uint32_t queueFamily = std::numeric_limits<uint32_t>::max();
    static inline vk::Queue queue;
    static inline vk::UniqueCommandPool commandPool;
    static inline vk::UniqueDescriptorPool descriptorPool;

    static inline Swapchain swapchain;
};
