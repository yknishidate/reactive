#include "Vulkan.hpp"
#include <iostream>
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
        std::cerr << pCallbackData->pMessage << std::endl;
        //assert(false);
        return VK_FALSE;
    }

    vk::Instance CreateInstance(const std::vector<const char*>& extensions,
                                const std::vector<const char*>& layers)
    {
        static vk::DynamicLoader dl;
        auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        vk::ApplicationInfo appInfo;
        appInfo.apiVersion = VK_API_VERSION_1_2;

        vk::InstanceCreateInfo InstanceCI{};
        InstanceCI.setPApplicationInfo(&appInfo);
        InstanceCI.setPEnabledExtensionNames(extensions);
        InstanceCI.setPEnabledLayerNames(layers);
        return vk::createInstance(InstanceCI);
    }

    vk::DebugUtilsMessengerEXT CreateDebugMessenger(vk::Instance instance)
    {
        vk::DebugUtilsMessengerCreateInfoEXT debugMessangerCI{};
        debugMessangerCI.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning);
        debugMessangerCI.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
        debugMessangerCI.setPfnUserCallback(&DebugMessage);
        return instance.createDebugUtilsMessengerEXT(debugMessangerCI);
    }

    uint32_t FindQueueFamily(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
    {
        std::vector queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
        for (uint32_t index = 0; index < queueFamilyProperties.size(); index++) {
            auto supportGraphics = queueFamilyProperties[index].queueFlags & vk::QueueFlagBits::eGraphics;
            auto supportCompute = queueFamilyProperties[index].queueFlags & vk::QueueFlagBits::eCompute;
            auto supportPresent = physicalDevice.getSurfaceSupportKHR(index, surface);
            if (supportGraphics && supportCompute && supportPresent) {
                return index;
            }
        }
        throw std::runtime_error("Failed to find queue family.");
    }

    vk::Device CreateDevice(vk::PhysicalDevice physicalDevice, uint32_t queueFamily)
    {
        float queuePriority = 0.0f;
        vk::DeviceQueueCreateInfo queueCI;
        queueCI.setQueueFamilyIndex(queueFamily);
        queueCI.setQueueCount(1);
        queueCI.setPQueuePriorities(&queuePriority);

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
        };

        vk::PhysicalDeviceFeatures deviceFeatures;
        deviceFeatures.shaderInt64 = true;
        vk::DeviceCreateInfo createInfo{ {}, queueCI, {}, deviceExtensions, &deviceFeatures };
        vk::StructureChain<vk::DeviceCreateInfo,
            vk::PhysicalDeviceBufferDeviceAddressFeatures,
            vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
            vk::PhysicalDeviceAccelerationStructureFeaturesKHR>
            createInfoChain{ createInfo, {true}, {true}, {true} };

        return physicalDevice.createDevice(createInfoChain.get<vk::DeviceCreateInfo>());
    }

    vk::SwapchainKHR CreateSwapchain()
    {
        vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
        swapchainCreateInfo.setSurface(Vulkan::Surface);
        swapchainCreateInfo.setMinImageCount(3);
        swapchainCreateInfo.setImageFormat(vk::Format::eB8G8R8A8Unorm);
        swapchainCreateInfo.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear);
        swapchainCreateInfo.setImageExtent({ static_cast<uint32_t>(Window::GetWidth()), static_cast<uint32_t>(Window::GetHeight()) });
        swapchainCreateInfo.setImageArrayLayers(1);
        swapchainCreateInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst);
        swapchainCreateInfo.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity);
        swapchainCreateInfo.setPresentMode(vk::PresentModeKHR::eFifo);
        swapchainCreateInfo.setClipped(true);
        return Vulkan::Device.createSwapchainKHR(swapchainCreateInfo);
    }

    std::vector<vk::ImageView> CreateImageViews()
    {
        vk::ImageViewCreateInfo info{};
        info.viewType = vk::ImageViewType::e2D;
        info.format = vk::Format::eB8G8R8A8Unorm;
        info.components.r = vk::ComponentSwizzle::eR;
        info.components.g = vk::ComponentSwizzle::eG;
        info.components.b = vk::ComponentSwizzle::eB;
        info.components.a = vk::ComponentSwizzle::eA;
        info.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

        std::vector<vk::ImageView> views;
        for (uint32_t i = 0; i < Vulkan::SwapchainImages.size(); i++) {
            info.image = Vulkan::SwapchainImages[i];
            views.push_back(Vulkan::Device.createImageView(info));
        }
        return views;
    }

    vk::CommandPool CreateCommandPool(vk::Device device, uint32_t queueFamily)
    {
        vk::CommandPoolCreateInfo commandPoolCI;
        commandPoolCI.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        commandPoolCI.setQueueFamilyIndex(queueFamily);
        return device.createCommandPool(commandPoolCI);
    }

    std::vector<vk::CommandBuffer> CreateCommandBuffers(vk::Device device, vk::CommandPool commandPool, uint32_t count)
    {
        vk::CommandBufferAllocateInfo commandBufferAI;
        commandBufferAI.setCommandPool(commandPool);
        commandBufferAI.setLevel(vk::CommandBufferLevel::ePrimary);
        commandBufferAI.setCommandBufferCount(count);
        return device.allocateCommandBuffers(commandBufferAI);
    }

    vk::CommandBuffer CreateCommandBuffer(vk::Device device, vk::CommandPool commandPool)
    {
        return std::move(CreateCommandBuffers(device, commandPool, 1).front());
    }

    vk::DescriptorPool CraeteDescriptorPool(vk::Device device)
    {
        std::vector<vk::DescriptorPoolSize> poolSizes{
            { vk::DescriptorType::eSampler, 100 },
            { vk::DescriptorType::eCombinedImageSampler, 100 },
            { vk::DescriptorType::eSampledImage, 100 },
            { vk::DescriptorType::eStorageImage, 100 },
            { vk::DescriptorType::eUniformBuffer, 100 },
            { vk::DescriptorType::eStorageBuffer, 100 },
            { vk::DescriptorType::eInputAttachment, 100 }
        };

        vk::DescriptorPoolCreateInfo descPoolCI;
        descPoolCI.setPoolSizes(poolSizes);
        descPoolCI.setMaxSets(100);
        descPoolCI.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
        return device.createDescriptorPool(descPoolCI);
    }
}

void Vulkan::Init()
{
    spdlog::info("Vulkan::Init()");
    std::vector layers{ "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
    Instance = CreateInstance(Window::GetExtensions(), layers);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(Instance);
    DebugMessenger = CreateDebugMessenger(Instance);
    PhysicalDevice = Instance.enumeratePhysicalDevices().front();
    Surface = Window::CreateSurface();
    QueueFamily = FindQueueFamily(PhysicalDevice, Surface);
    Device = CreateDevice(PhysicalDevice, QueueFamily);
    Queue = Device.getQueue(QueueFamily, 0);
    CommandPool = CreateCommandPool(Device, QueueFamily);
    Swapchain = CreateSwapchain();
    SwapchainImages = Device.getSwapchainImagesKHR(Swapchain);
    SwapchainImageViews = CreateImageViews();
    DescriptorPool = CraeteDescriptorPool(Device);
}

std::vector<vk::UniqueCommandBuffer> Vulkan::AllocateCommandBuffers(uint32_t count)
{
    vk::CommandBufferAllocateInfo commandBufferInfo;
    commandBufferInfo.setCommandPool(Vulkan::CommandPool);
    commandBufferInfo.setLevel(vk::CommandBufferLevel::ePrimary);
    commandBufferInfo.setCommandBufferCount(count);
    return Vulkan::Device.allocateCommandBuffersUnique(commandBufferInfo);
}

vk::UniqueCommandBuffer Vulkan::AllocateCommandBuffer()
{
    return std::move(AllocateCommandBuffers(1).front());
}

void Vulkan::SubmitAndWait(vk::CommandBuffer commandBuffer)
{
    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(commandBuffer);
    Queue.submit(submitInfo);
    Queue.waitIdle();
}

void Vulkan::OneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command)
{
    vk::UniqueCommandBuffer commandBuffer = AllocateCommandBuffer();

    vk::CommandBufferBeginInfo beginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    commandBuffer->begin(beginInfo);
    command(*commandBuffer);
    commandBuffer->end();
    SubmitAndWait(*commandBuffer);
}

uint32_t Vulkan::FindMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags memoryProp)
{
    vk::PhysicalDeviceMemoryProperties memoryProperties = Vulkan::PhysicalDevice.getMemoryProperties();
    for (uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index) {
        auto propertyFlags = memoryProperties.memoryTypes[index].propertyFlags;
        bool match = (propertyFlags & memoryProp) == memoryProp;
        if (requirements.memoryTypeBits & (1 << index) && match) {
            return index;
        }
    }
    throw std::runtime_error("Failed to find memory type index.");
}
