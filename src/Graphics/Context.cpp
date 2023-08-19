#include "Graphics/Context.hpp"

#include "Graphics/Accel.hpp"
#include "Graphics/DescriptorSet.hpp"
#include "Graphics/Image.hpp"
#include "Graphics/Pipeline.hpp"
#include "Graphics/Shader.hpp"
#include "Timer/GPUTimer.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace rv {
vk::BufferUsageFlags getBufferUsage(rv::BufferUsage usage) {
    switch (usage) {
        case rv::BufferUsage::Uniform:
            return vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst |
                   vk::BufferUsageFlagBits::eShaderDeviceAddress;
        case rv::BufferUsage::Storage:
            return vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst |
                   vk::BufferUsageFlagBits::eShaderDeviceAddress;
        case rv::BufferUsage::Staging:
            return vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;
        case rv::BufferUsage::Vertex:
            return vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                   vk::BufferUsageFlagBits::eStorageBuffer |
                   vk::BufferUsageFlagBits::eShaderDeviceAddress |
                   vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
        case rv::BufferUsage::Index:
            return vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                   vk::BufferUsageFlagBits::eStorageBuffer |
                   vk::BufferUsageFlagBits::eShaderDeviceAddress |
                   vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
        case rv::BufferUsage::Indirect:
            return vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst |
                   vk::BufferUsageFlagBits::eIndirectBuffer;
        case rv::BufferUsage::AccelStorage:
            return vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
                   vk::BufferUsageFlagBits::eShaderDeviceAddress;
        case rv::BufferUsage::AccelInput:
            return vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                   vk::BufferUsageFlagBits::eStorageBuffer |
                   vk::BufferUsageFlagBits::eShaderDeviceAddress |
                   vk::BufferUsageFlagBits::eTransferDst;
        case rv::BufferUsage::ShaderBindingTable:
            return vk::BufferUsageFlagBits::eShaderBindingTableKHR |
                   vk::BufferUsageFlagBits::eShaderDeviceAddress |
                   vk::BufferUsageFlagBits::eTransferDst;
        case rv::BufferUsage::Scratch:
            return vk::BufferUsageFlagBits::eStorageBuffer |
                   vk::BufferUsageFlagBits::eShaderDeviceAddress;
    }
}

vk::MemoryPropertyFlags getMemoryProperty(rv::MemoryUsage usage) {
    switch (usage) {
        case rv::MemoryUsage::Device:
            return vk::MemoryPropertyFlagBits::eDeviceLocal;
        case rv::MemoryUsage::Host:
            return vk::MemoryPropertyFlagBits::eHostVisible |
                   vk::MemoryPropertyFlagBits::eHostCoherent;
        case rv::MemoryUsage::DeviceHost:
            return vk::MemoryPropertyFlagBits::eDeviceLocal |
                   vk::MemoryPropertyFlagBits::eHostVisible |
                   vk::MemoryPropertyFlagBits::eHostCoherent;
    }
}

vk::ImageUsageFlags getImageUsage(rv::ImageUsage usage) {
    switch (usage) {
        case rv::ImageUsage::ColorAttachment:
            return vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc |
                   vk::ImageUsageFlagBits::eTransferDst;
        case rv::ImageUsage::DepthAttachment:
            return vk::ImageUsageFlagBits::eDepthStencilAttachment |
                   vk::ImageUsageFlagBits::eTransferDst;
        case rv::ImageUsage::DepthStencilAttachment:
            return vk::ImageUsageFlagBits::eDepthStencilAttachment |
                   vk::ImageUsageFlagBits::eTransferDst;
        case rv::ImageUsage::Sampled:
            return vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst |
                   vk::ImageUsageFlagBits::eTransferSrc;
        case rv::ImageUsage::Storage:
            return vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst |
                   vk::ImageUsageFlagBits::eTransferSrc;
    }
}

vk::ImageAspectFlags getImageAspect(rv::ImageUsage usage) {
    switch (usage) {
        case rv::ImageUsage::ColorAttachment:
            return vk::ImageAspectFlagBits::eColor;
        case rv::ImageUsage::DepthAttachment:
            return vk::ImageAspectFlagBits::eDepth;
        case rv::ImageUsage::DepthStencilAttachment:
            return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        case rv::ImageUsage::Sampled:
            return vk::ImageAspectFlagBits::eColor;
        case rv::ImageUsage::Storage:
            return vk::ImageAspectFlagBits::eColor;
    }
}

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

    checkDeviceExtensionSupport(deviceExtensions);

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
    vk::ImageUsageFlags usage = getImageUsage(createInfo.usage);
    vk::ImageAspectFlags aspect = getImageAspect(createInfo.usage);
    return std::make_shared<Image>(this, usage, createInfo.width, createInfo.height,
                                   createInfo.depth, createInfo.format, createInfo.layout, aspect,
                                   createInfo.mipLevels);
}

BufferHandle Context::createBuffer(BufferCreateInfo createInfo) const {
    vk::BufferUsageFlags usage = getBufferUsage(createInfo.usage);
    vk::MemoryPropertyFlags memory = getMemoryProperty(createInfo.memory);
    return std::make_shared<Buffer>(this, usage, memory, createInfo.size, createInfo.data);
}

MeshHandle Context::createMesh(MeshCreateInfo createInfo) const {
    return std::make_shared<Mesh>(this, createInfo);
}

MeshHandle Context::createSphereMesh(SphereMeshCreateInfo createInfo) const {
    return std::make_shared<Mesh>(this, createInfo);
}

MeshHandle Context::createPlaneMesh(PlaneMeshCreateInfo createInfo) const {
    return std::make_shared<Mesh>(this, createInfo);
}

MeshHandle Context::createCubeMesh(CubeMeshCreateInfo createInfo) const {
    return std::make_shared<Mesh>(this, createInfo);
}

MeshHandle Context::createCubeLineMesh(CubeLineMeshCreateInfo createInfo) const {
    return std::make_shared<Mesh>(this, createInfo);
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
