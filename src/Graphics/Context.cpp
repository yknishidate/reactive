#include "Graphics/Context.hpp"

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

void Context::initDevice(const std::vector<const char*>& deviceExtensions,
                         const vk::PhysicalDeviceFeatures& deviceFeatures,
                         const void* deviceCreateInfoPNext,
                         bool enableRayTracing) {
    // Create device
    float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo queueInfo;
    queueInfo.setQueueFamilyIndex(queueFamily);
    queueInfo.setQueueCount(1);
    queueInfo.setPQueuePriorities(&queuePriority);

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

CommandBufferHandle Context::allocateCommandBuffer() const {
    vk::CommandBufferAllocateInfo commandBufferInfo;
    commandBufferInfo.setCommandPool(*commandPool);
    commandBufferInfo.setLevel(vk::CommandBufferLevel::ePrimary);
    commandBufferInfo.setCommandBufferCount(1);

    vk::CommandBuffer _commandBuffer = device->allocateCommandBuffers(commandBufferInfo).front();
    return std::make_shared<CommandBuffer>(this, _commandBuffer);
}

void Context::submit(CommandBufferHandle commandBuffer, vk::Fence fence) const {
    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(*commandBuffer->commandBuffer);
    queue.submit(submitInfo, fence);
}

void Context::oneTimeSubmit(const std::function<void(CommandBufferHandle)>& command) const {
    vk::CommandBufferAllocateInfo commandBufferInfo;
    commandBufferInfo.setCommandPool(*commandPool);
    commandBufferInfo.setLevel(vk::CommandBufferLevel::ePrimary);
    commandBufferInfo.setCommandBufferCount(1);

    vk::CommandBuffer _commandBuffer = device->allocateCommandBuffers(commandBufferInfo).front();

    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    _commandBuffer.begin(beginInfo);

    CommandBufferHandle commandBuffer = std::make_shared<CommandBuffer>(this, _commandBuffer);
    command(commandBuffer);

    _commandBuffer.end();

    queue.submit(vk::SubmitInfo().setCommandBuffers(_commandBuffer));
    queue.waitIdle();
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
