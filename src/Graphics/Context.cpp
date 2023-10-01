#include "Graphics/Context.hpp"

#include <ranges>

#include "Graphics/Accel.hpp"
#include "Graphics/CommandBuffer.hpp"
#include "Graphics/DescriptorSet.hpp"
#include "Graphics/Image.hpp"
#include "Graphics/Pipeline.hpp"
#include "Graphics/Shader.hpp"
#include "Timer/GPUTimer.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace rv {
void Context::initInstance(bool enableValidation,
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

    spdlog::info("Enabled layers:");
    for (auto& layer : layers) {
        spdlog::info("  {}", layer);
    }

    spdlog::info("Enabled instance extensions:");
    for (auto& extension : instanceExtensions) {
        spdlog::info("  {}", extension);
    }

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

void Context::initPhysicalDevice(vk::SurfaceKHR surface) {
    spdlog::info("Context::initPhysicalDevice");

    // Select discrete gpu
    for (auto gpu : instance->enumeratePhysicalDevices()) {
        if (gpu.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            physicalDevice = gpu;
        }
    }
    // If discrete gpu not found, select first gpu
    if (!physicalDevice) {
        physicalDevice = instance->enumeratePhysicalDevices().front();
    }
    spdlog::info("Selected GPU: {}", std::string(physicalDevice.getProperties().deviceName.data()));

    // Find queue family
    spdlog::info("Selected queue families:");
    std::vector properties = physicalDevice.getQueueFamilyProperties();
    for (uint32_t index = 0; index < properties.size(); index++) {
        auto supportGraphics = properties[index].queueFlags & vk::QueueFlagBits::eGraphics;
        auto supportCompute = properties[index].queueFlags & vk::QueueFlagBits::eCompute;
        auto supportTransfer = properties[index].queueFlags & vk::QueueFlagBits::eTransfer;
        if (surface) {
            auto supportPresent = physicalDevice.getSurfaceSupportKHR(index, surface);
            if (supportGraphics && supportCompute && supportPresent && supportTransfer) {
                if (!queueFamilies.contains(QueueFlags::General)) {
                    queueFamilies[QueueFlags::General] = index;
                    spdlog::info("  General: {} x {}", index, properties[index].queueCount);
                    continue;
                }
            }
            if (supportGraphics && supportPresent) {
                if (!queueFamilies.contains(QueueFlags::Graphics)) {
                    queueFamilies[QueueFlags::Graphics] = index;
                    spdlog::info("  Graphics: {} x {}", index, properties[index].queueCount);
                    continue;
                }
            }
        } else {
            if (supportGraphics && supportCompute && supportTransfer) {
                if (!queueFamilies.contains(QueueFlags::General)) {
                    queueFamilies[QueueFlags::General] = index;
                    spdlog::info("  General: {} x {}", index, properties[index].queueCount);
                    continue;
                }
            }
            if (supportGraphics) {
                if (!queueFamilies.contains(QueueFlags::Graphics)) {
                    queueFamilies[QueueFlags::Graphics] = index;
                    spdlog::info("  Graphics: {} x {}", index, properties[index].queueCount);
                    continue;
                }
            }
        }

        // These are not related to surface
        if (supportCompute) {
            if (!queueFamilies.contains(QueueFlags::Compute)) {
                queueFamilies[QueueFlags::Compute] = index;
                spdlog::info("  Compute: {} x {}", index, properties[index].queueCount);
                continue;
            }
        }
        if (supportTransfer) {
            if (!queueFamilies.contains(QueueFlags::Transfer)) {
                queueFamilies[QueueFlags::Transfer] = index;
                spdlog::info("  Transfer: {} x {}", index, properties[index].queueCount);
            }
        }
    }

    if (!queueFamilies.contains(QueueFlags::General)) {
        throw std::runtime_error("Failed to find general queue family.");
    }
}

void Context::initDevice(const std::vector<const char*>& deviceExtensions,
                         const vk::PhysicalDeviceFeatures& deviceFeatures,
                         const void* deviceCreateInfoPNext,
                         bool enableRayTracing) {
    // Create device
    float queuePriority = 1.0f;
    std::unordered_map<vk::QueueFlags, std::vector<float>> queuePriorities;
    std::vector<vk::DeviceQueueCreateInfo> queueInfo;
    for (const auto& [flag, queueFamily] : queueFamilies) {
        uint32_t queueCount = physicalDevice.getQueueFamilyProperties()[queueFamily].queueCount;
        queuePriorities[flag] = {};
        queuePriorities[flag].resize(queueCount);
        std::ranges::fill(queuePriorities[flag], 1.0f);

        vk::DeviceQueueCreateInfo info;
        info.setQueueFamilyIndex(queueFamily);
        info.setQueuePriorities(queuePriorities[flag]);
        queueInfo.push_back(info);
        queues[flag] = {};
        queues.at(flag).resize(queueCount);
    }

    checkDeviceExtensionSupport(deviceExtensions);

    vk::DeviceCreateInfo deviceInfo;
    deviceInfo.setQueueCreateInfos(queueInfo);
    deviceInfo.setPEnabledExtensionNames(deviceExtensions);
    deviceInfo.setPEnabledFeatures(&deviceFeatures);
    deviceInfo.setPNext(deviceCreateInfoPNext);
    device = physicalDevice.createDeviceUnique(deviceInfo);

    spdlog::info("Enabled device extensions:");
    for (auto& extension : deviceExtensions) {
        spdlog::info("  {}", extension);
    }

    // Get queue
    for (const auto& [flag, queueFamily] : queueFamilies) {
        for (int i = 0; i < queues[flag].size(); i++) {
            queues[flag][i] = device->getQueue(queueFamily, i);
        }
        maxQueueCount = std::min(maxQueueCount, static_cast<uint32_t>(queues[flag].size()));
    }

    // Create command pool
    for (const auto& [flag, queueFamily] : queueFamilies) {
        vk::CommandPoolCreateInfo commandPoolCreateInfo;
        commandPoolCreateInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        commandPoolCreateInfo.setQueueFamilyIndex(queueFamily);
        commandPools[flag] = std::vector<vk::UniqueCommandPool>(queues[flag].size());

        for (int i = 0; i < queues[flag].size(); i++) {
            commandPools[flag][i] =
                std::move(device->createCommandPoolUnique(commandPoolCreateInfo));
        }
    }

    // Create descriptor pool
    std::vector<vk::DescriptorPoolSize> poolSizes{
        {vk::DescriptorType::eSampler, 100},
        {vk::DescriptorType::eCombinedImageSampler, 100},
        {vk::DescriptorType::eSampledImage, 100},
        {vk::DescriptorType::eStorageImage, 100},
        {vk::DescriptorType::eUniformBuffer, 100},
        {vk::DescriptorType::eStorageBuffer, 100},
        {vk::DescriptorType::eInputAttachment, 100},
    };
    if (enableRayTracing) {
        poolSizes.emplace_back(vk::DescriptorType::eAccelerationStructureKHR, 100);
    }

    vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
    descriptorPoolCreateInfo.setPoolSizes(poolSizes);
    descriptorPoolCreateInfo.setMaxSets(100);
    descriptorPoolCreateInfo.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    descriptorPool = device->createDescriptorPoolUnique(descriptorPoolCreateInfo);
}

vk::Queue Context::getQueue(vk::QueueFlags flag) const {
    return queues.at(flag)[getQueueIndexByThreadId()];
}

uint32_t Context::getQueueFamily(vk::QueueFlags flag) const {
    return queueFamilies.at(flag);
}

CommandBufferHandle Context::allocateCommandBuffer(vk::QueueFlags flag) const {
    uint32_t queueIndex = getQueueIndexByThreadId();

    vk::CommandPool commandPool = *commandPools.at(flag)[queueIndex];
    vk::CommandBufferAllocateInfo commandBufferInfo;
    commandBufferInfo.setCommandPool(commandPool);
    commandBufferInfo.setLevel(vk::CommandBufferLevel::ePrimary);
    commandBufferInfo.setCommandBufferCount(1);

    vk::CommandBuffer commandBuffer = device->allocateCommandBuffers(commandBufferInfo).front();
    return std::make_shared<CommandBuffer>(this, commandBuffer, commandPool, flag);
}

void Context::submit(CommandBufferHandle commandBuffer, vk::Fence fence) const {
    uint32_t queueIndex = getQueueIndexByThreadId();

    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(*commandBuffer->commandBuffer);
    queues.at(commandBuffer->getQueueFlags())[queueIndex].submit(submitInfo, fence);
}

void Context::oneTimeSubmit(const std::function<void(CommandBufferHandle)>& command,
                            vk::QueueFlags flag) const {
    uint32_t queueIndex = getQueueIndexByThreadId();
    CommandBufferHandle commandBuffer = allocateCommandBuffer(flag);
    commandBuffer->begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    command(commandBuffer);
    commandBuffer->end();
    submit(commandBuffer);
    queues.at(flag)[queueIndex].waitIdle();
}

vk::UniqueDescriptorSet Context::allocateDescriptorSet(
    vk::DescriptorSetLayout descSetLayout) const {
    vk::DescriptorSetAllocateInfo descSetInfo;
    descSetInfo.setDescriptorPool(*descriptorPool);
    descSetInfo.setSetLayouts(descSetLayout);
    return std::move(device->allocateDescriptorSetsUnique(descSetInfo).front());
}

uint32_t Context::findMemoryTypeIndex(vk::MemoryRequirements requirements,
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

ShaderHandle Context::createShader(ShaderCreateInfo createInfo) const {
    return std::make_shared<Shader>(this, createInfo);
}

DescriptorSetHandle Context::createDescriptorSet(DescriptorSetCreateInfo createInfo) const {
    return std::make_shared<DescriptorSet>(this, createInfo);
}

GraphicsPipelineHandle Context::createGraphicsPipeline(
    GraphicsPipelineCreateInfo createInfo) const {
    return std::make_shared<GraphicsPipeline>(this, createInfo);
}

MeshShaderPipelineHandle Context::createMeshShaderPipeline(
    MeshShaderPipelineCreateInfo createInfo) const {
    return std::make_shared<MeshShaderPipeline>(this, createInfo);
}

ComputePipelineHandle Context::createComputePipeline(ComputePipelineCreateInfo createInfo) const {
    return std::make_shared<ComputePipeline>(this, createInfo);
}

RayTracingPipelineHandle Context::createRayTracingPipeline(
    RayTracingPipelineCreateInfo createInfo) const {
    return std::make_shared<RayTracingPipeline>(this, createInfo);
}

ImageHandle Context::createImage(ImageCreateInfo createInfo) const {
    return std::make_shared<Image>(this, createInfo.usage, createInfo.extent, createInfo.format,
                                   createInfo.aspect, createInfo.mipLevels, createInfo.debugName);
}

BufferHandle Context::createBuffer(BufferCreateInfo createInfo) const {
    return std::make_shared<Buffer>(this, createInfo.usage, createInfo.memory, createInfo.size,
                                    createInfo.data, createInfo.debugName);
}

BottomAccelHandle Context::createBottomAccel(BottomAccelCreateInfo createInfo) const {
    return std::make_shared<BottomAccel>(this, createInfo);
}

TopAccelHandle Context::createTopAccel(TopAccelCreateInfo createInfo) const {
    return std::make_shared<TopAccel>(this, createInfo);
}

GPUTimerHandle Context::createGPUTimer(GPUTimerCreateInfo createInfo) const {
    return std::make_shared<GPUTimer>(this, createInfo);
}
}  // namespace rv
