#include "Context.hpp"
#include <iostream>
#include <regex>
#include <spdlog/spdlog.h>
#include "Window.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace
{
    VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                                                void* pUserData)
    {
        const std::string message{ pCallbackData->pMessage };
        const std::regex regex{ "The Vulkan spec states: " };
        std::smatch result;
        if (std::regex_search(message, result, regex)) {
            spdlog::error("{}\n {}\n", message.substr(0, result.position()), message.substr(result.position()));
        } else {
            spdlog::error("{}\n", message);
        }
        //assert(false);
        return VK_FALSE;
    }

    vk::UniqueInstance CreateInstance(const std::vector<const char*>& extensions,
                                      const std::vector<const char*>& layers)
    {
        static const vk::DynamicLoader dl;
        const auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        const auto appInfo = vk::ApplicationInfo()
            .setApiVersion(VK_API_VERSION_1_2);

        return vk::createInstanceUnique(
            vk::InstanceCreateInfo()
            .setPApplicationInfo(&appInfo)
            .setPEnabledExtensionNames(extensions)
            .setPEnabledLayerNames(layers)
        );
    }

    vk::UniqueDebugUtilsMessengerEXT CreateDebugMessenger(vk::Instance instance)
    {
        return instance.createDebugUtilsMessengerEXTUnique(
            vk::DebugUtilsMessengerCreateInfoEXT()
            .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
            .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
            .setPfnUserCallback(&DebugMessage)
        );
    }

    uint32_t FindQueueFamily(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
    {
        const std::vector properties = physicalDevice.getQueueFamilyProperties();
        for (uint32_t index = 0; index < properties.size(); index++) {
            const auto supportGraphics = properties[index].queueFlags & vk::QueueFlagBits::eGraphics;
            const auto supportCompute = properties[index].queueFlags & vk::QueueFlagBits::eCompute;
            const auto supportPresent = physicalDevice.getSurfaceSupportKHR(index, surface);
            if (supportGraphics && supportCompute && supportPresent) {
                return index;
            }
        }
        throw std::runtime_error("Failed to find queue family.");
    }

    vk::UniqueDevice CreateDevice(vk::PhysicalDevice physicalDevice, uint32_t queueFamily)
    {
        const float queuePriority = 0.0f;
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

        const auto deviceFeatures = vk::PhysicalDeviceFeatures()
            .setShaderInt64(true)
            .setFragmentStoresAndAtomics(true)
            .setVertexPipelineStoresAndAtomics(true);

        const auto descFeatures = vk::PhysicalDeviceDescriptorIndexingFeatures()
            .setRuntimeDescriptorArray(true);

        const auto deviceInfo = vk::DeviceCreateInfo()
            .setQueueCreateInfos(queueInfo)
            .setPEnabledExtensionNames(deviceExtensions)
            .setPEnabledFeatures(&deviceFeatures);

        vk::StructureChain<vk::DeviceCreateInfo,
            vk::PhysicalDeviceBufferDeviceAddressFeatures,
            vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
            vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
            vk::PhysicalDeviceDescriptorIndexingFeatures>
            createInfoChain{ deviceInfo, {true}, {true}, {true}, descFeatures };

        return physicalDevice.createDeviceUnique(createInfoChain.get<vk::DeviceCreateInfo>());
    }

    vk::UniqueCommandPool CreateCommandPool(vk::Device device, uint32_t queueFamily)
    {
        return device.createCommandPoolUnique(
            vk::CommandPoolCreateInfo()
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
            .setQueueFamilyIndex(queueFamily)
        );
    }

    vk::UniqueDescriptorPool CraeteDescriptorPool(vk::Device device)
    {
        const std::vector<vk::DescriptorPoolSize> poolSizes{
            { vk::DescriptorType::eSampler, 100 },
            { vk::DescriptorType::eCombinedImageSampler, 100 },
            { vk::DescriptorType::eSampledImage, 100 },
            { vk::DescriptorType::eStorageImage, 100 },
            { vk::DescriptorType::eUniformBuffer, 100 },
            { vk::DescriptorType::eStorageBuffer, 100 },
            { vk::DescriptorType::eInputAttachment, 100 }
        };

        return device.createDescriptorPoolUnique(
            vk::DescriptorPoolCreateInfo()
            .setPoolSizes(poolSizes)
            .setMaxSets(100)
            .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
        );
    }

    std::vector<const char*> GetExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        return extensions;
    }

    vk::UniqueSurfaceKHR CreateSurface(vk::Instance instance)
    {
        VkSurfaceKHR _surface;
        if (glfwCreateWindowSurface(VkInstance{ instance }, Window::GetWindow(), nullptr, &_surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
        return vk::UniqueSurfaceKHR{ _surface, {instance} };
    }
}

void Context::Init()
{
    spdlog::info("Context::Init()");
    std::vector layers{ "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
    instance = CreateInstance(GetExtensions(), layers);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);
    debugMessenger = CreateDebugMessenger(*instance);
    physicalDevice = instance->enumeratePhysicalDevices().front();
    surface = CreateSurface(*instance);
    queueFamily = FindQueueFamily(physicalDevice, *surface);
    device = CreateDevice(physicalDevice, queueFamily);
    queue = device->getQueue(queueFamily, 0);
    commandPool = CreateCommandPool(*device, queueFamily);
    //swapchain = CreateSwapchain(*device, *surface, minImageCount, queueFamily);
    //swapchainImages = device->getSwapchainImagesKHR(*swapchain);
    //swapchainImageViews = CreateImageViews(*device, swapchainImages);
    swapchain = Swapchain{ *device, *surface, queueFamily };
    descriptorPool = CraeteDescriptorPool(*device);
    //renderPass = CreateRenderPass(*device);

    //// Create Command Buffers
    //size_t imageCount = swapchainImages.size();
    //frames = std::vector<Frame>(imageCount);
    //frameSemaphores = std::vector<FrameSemaphores>(imageCount);
    //for (uint32_t i = 0; i < imageCount; i++) {
    //    frames[i] = Frame{ *renderPass, *swapchainImageViews[i] };
    //}
}

std::vector<vk::UniqueCommandBuffer> Context::AllocateCommandBuffers(uint32_t count)
{
    return device->allocateCommandBuffersUnique(
        vk::CommandBufferAllocateInfo()
        .setCommandPool(*commandPool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(count)
    );
}

vk::UniqueCommandBuffer Context::AllocateCommandBuffer()
{
    return std::move(AllocateCommandBuffers(1).front());
}

void Context::SubmitAndWait(vk::CommandBuffer commandBuffer)
{
    queue.submit(
        vk::SubmitInfo()
        .setCommandBuffers(commandBuffer)
    );
    queue.waitIdle();
}

void Context::OneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command)
{
    vk::UniqueCommandBuffer commandBuffer = AllocateCommandBuffer();

    commandBuffer->begin(
        vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
    );
    command(*commandBuffer);
    commandBuffer->end();

    SubmitAndWait(*commandBuffer);
}

uint32_t Context::FindMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags memoryProp)
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

void Context::BeginRenderPass()
{
    swapchain.BeginRenderPass();
}

void Context::EndRenderPass()
{
    swapchain.EndRenderPass();
}

void Context::WaitNextFrame()
{
    swapchain.WaitNextFrame(*device);
}

vk::CommandBuffer Context::BeginCommandBuffer()
{
    return swapchain.BeginCommandBuffer();
}

void Context::Submit()
{
    swapchain.Submit(queue);
}

void Context::Present()
{
    swapchain.Present(queue);
}
