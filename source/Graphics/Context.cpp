#include "Context.hpp"
#include "Accel.hpp"
#include "DescriptorSet.hpp"
#include "Pipeline.hpp"
#include "Shader.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

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
    };
    if (enableRayTracing) {
        poolSizes.push_back({vk::DescriptorType::eAccelerationStructureKHR, 100});
    }

    vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
    descriptorPoolCreateInfo.setPoolSizes(poolSizes);
    descriptorPoolCreateInfo.setMaxSets(100);
    descriptorPoolCreateInfo.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    descriptorPool = device->createDescriptorPoolUnique(descriptorPoolCreateInfo);
}

std::vector<vk::UniqueCommandBuffer> Context::allocateCommandBuffers(uint32_t count) const {
    return device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo()
                                                    .setCommandPool(*commandPool)
                                                    .setLevel(vk::CommandBufferLevel::ePrimary)
                                                    .setCommandBufferCount(count));
}

void Context::oneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command) const {
    vk::UniqueCommandBuffer commandBuffer = std::move(allocateCommandBuffers(1).front());

    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    commandBuffer->begin(beginInfo);

    command(*commandBuffer);

    commandBuffer->end();

    queue.submit(vk::SubmitInfo().setCommandBuffers(*commandBuffer));
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

Shader Context::createShader(ShaderCreateInfo createInfo) const {
    return {this, createInfo};
}

DescriptorSet Context::createDescriptorSet(DescriptorSetCreateInfo createInfo) const {
    return {this, createInfo};
}

GraphicsPipeline Context::createGraphicsPipeline(GraphicsPipelineCreateInfo createInfo) const {
    return {this, createInfo};
}

HostBuffer Context::createHostBuffer(BufferCreateInfo createInfo) const {
    return {this, createInfo};
}

DeviceBuffer Context::createDeviceBuffer(BufferCreateInfo createInfo) const {
    return {this, createInfo};
}

Mesh Context::createMesh(MeshCreateInfo createInfo) const {
    return {this, createInfo};
}

Mesh Context::createSphereMesh(SphereMeshCreateInfo createInfo) const {
    return {this, createInfo};
}

Mesh Context::createPlaneMesh(PlaneMeshCreateInfo createInfo) const {
    return {this, createInfo};
}

Mesh Context::createCubeMesh(CubeMeshCreateInfo createInfo) const {
    return {this, createInfo};
}

BottomAccel Context::createBottomAccel(BottomAccelCreateInfo createInfo) const {
    return {this, createInfo};
}
