#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <vector>

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

class Context {
public:
    void initInstance(bool enableValidation,
                      const std::vector<const char*>& layers,
                      const std::vector<const char*>& instanceExtensions,
                      uint32_t apiVersion) {
        // Setup dynamic loader
        static const vk::DynamicLoader dl;
        auto vkGetInstanceProcAddr =
            dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        // Create instance
        vk::ApplicationInfo appInfo;
        appInfo.setApiVersion(apiVersion);
        instance = vk::createInstanceUnique(vk::InstanceCreateInfo()
                                                .setPApplicationInfo(&appInfo)
                                                .setPEnabledExtensionNames(instanceExtensions)
                                                .setPEnabledLayerNames(layers));
        VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

        // Create debug messenger
        if (enableValidation) {
            debugMessenger = instance->createDebugUtilsMessengerEXTUnique(
                vk::DebugUtilsMessengerCreateInfoEXT()
                    .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                                        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
                    .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
                    .setPfnUserCallback(&debugCallback));
        }
    }

    void initPhysicalDevice(vk::SurfaceKHR surface) {
        // Pick GPU
        physicalDevice = instance->enumeratePhysicalDevices().front();

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
        if (queueFamily == ~0u) {
            throw std::runtime_error("Failed to find queue family.");
        }
    }

    void initDevice(const std::vector<const char*>& deviceExtensions,
                    const vk::PhysicalDeviceFeatures& deviceFeatures,
                    const void* deviceCreateInfoPNext) {
        // Create device
        float queuePriority = 1.0f;
        vk::DeviceQueueCreateInfo queueInfo;
        queueInfo.setQueueFamilyIndex(queueFamily);
        queueInfo.setQueueCount(1);
        queueInfo.setPQueuePriorities(&queuePriority);

        vk::DeviceCreateInfo deviceInfo;
        deviceInfo.setQueueCreateInfos(queueInfo);
        deviceInfo.setPEnabledExtensionNames(deviceExtensions);
        deviceInfo.setPEnabledFeatures(&deviceFeatures);
        deviceInfo.setPNext(deviceCreateInfoPNext);
        device = physicalDevice.createDeviceUnique(deviceInfo);

        // Get queue
        queue = device->getQueue(queueFamily, 0);

        // Create command pool
        vk::CommandPoolCreateInfo commandPoolCreateInfo;
        commandPoolCreateInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        commandPoolCreateInfo.setQueueFamilyIndex(queueFamily);
        commandPool = device->createCommandPoolUnique(commandPoolCreateInfo);

        // Create descriptor pool
        std::vector<vk::DescriptorPoolSize> poolSizes{
            {vk::DescriptorType::eSampler, 100},
            {vk::DescriptorType::eCombinedImageSampler, 100},
            {vk::DescriptorType::eSampledImage, 100},
            {vk::DescriptorType::eStorageImage, 100},
            {vk::DescriptorType::eUniformBuffer, 100},
            {vk::DescriptorType::eStorageBuffer, 100},
            {vk::DescriptorType::eInputAttachment, 100},
            {vk::DescriptorType::eAccelerationStructureKHR, 100}};

        vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
        descriptorPoolCreateInfo.setPoolSizes(poolSizes);
        descriptorPoolCreateInfo.setMaxSets(100);
        descriptorPoolCreateInfo.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
        descriptorPool = device->createDescriptorPoolUnique(descriptorPoolCreateInfo);
    }

    vk::Instance getInstance() const { return *instance; }
    vk::Device getDevice() const { return *device; }
    vk::PhysicalDevice getPhysicalDevice() const { return physicalDevice; }
    vk::Queue getQueue() const { return queue; }
    uint32_t getQueueFamily() const { return queueFamily; }
    vk::DescriptorPool getDescriptorPool() const { return *descriptorPool; }

    std::vector<vk::UniqueCommandBuffer> allocateCommandBuffers(uint32_t count) const {
        return device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo()
                                                        .setCommandPool(*commandPool)
                                                        .setLevel(vk::CommandBufferLevel::ePrimary)
                                                        .setCommandBufferCount(count));
    }

    void oneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command) const {
        vk::UniqueCommandBuffer commandBuffer = std::move(allocateCommandBuffers(1).front());

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        commandBuffer->begin(beginInfo);

        command(*commandBuffer);

        commandBuffer->end();

        queue.submit(vk::SubmitInfo().setCommandBuffers(*commandBuffer));
        queue.waitIdle();
    }

    vk::UniqueDescriptorSet allocateDescriptorSet(vk::DescriptorSetLayout descSetLayout) const {
        vk::DescriptorSetAllocateInfo descSetInfo;
        descSetInfo.setDescriptorPool(*descriptorPool);
        descSetInfo.setSetLayouts(descSetLayout);
        return std::move(device->allocateDescriptorSetsUnique(descSetInfo).front());
    }

    uint32_t findMemoryTypeIndex(vk::MemoryRequirements requirements,
                                 vk::MemoryPropertyFlags memoryProp) const {
        vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
        for (uint32_t i = 0; i != memProperties.memoryTypeCount; ++i) {
            if ((requirements.memoryTypeBits & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & memoryProp) == memoryProp) {
                return i;
            }
        }
        throw std::runtime_error("Failed to find memory type index.");
    }

private:
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                  VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                  VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                  void* pUserData) {
        const std::string message{pCallbackData->pMessage};
        const std::regex regex{"The Vulkan spec states: "};
        std::smatch result;
        if (std::regex_search(message, result, regex)) {
            spdlog::error("{}\n", message.substr(0, result.position()));
        } else {
            spdlog::error("{}\n", message);
        }
        return VK_FALSE;
    }

    vk::UniqueInstance instance;
    vk::UniqueDebugUtilsMessengerEXT debugMessenger;
    vk::UniqueDevice device;
    vk::PhysicalDevice physicalDevice;
    uint32_t queueFamily = ~0u;
    vk::Queue queue;
    vk::UniqueCommandPool commandPool;
    vk::UniqueDescriptorPool descriptorPool;
};
