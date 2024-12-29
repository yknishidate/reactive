#include "reactive/Graphics/Context.hpp"

#include <ranges>

#include "reactive/Graphics/Accel.hpp"
#include "reactive/Graphics/CommandBuffer.hpp"
#include "reactive/Graphics/DescriptorSet.hpp"
#include "reactive/Graphics/Fence.hpp"
#include "reactive/Graphics/Image.hpp"
#include "reactive/Graphics/Pipeline.hpp"
#include "reactive/Graphics/Shader.hpp"
#include "reactive/Timer/GPUTimer.hpp"

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
    m_instance = vk::createInstanceUnique(vk::InstanceCreateInfo()
                                            .setPApplicationInfo(&appInfo)
                                            .setPEnabledExtensionNames(instanceExtensions)
                                            .setPEnabledLayerNames(layers));
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance);

    spdlog::info("Enabled layers:");
    for (auto& layer : layers) {
        spdlog::info("  {}", layer);
    }

    spdlog::info("Enabled m_instance extensions:");
    for (auto& extension : instanceExtensions) {
        spdlog::info("  {}", extension);
    }

    // Create debug messenger
    if (enableValidation) {
        m_debugMessenger = m_instance->createDebugUtilsMessengerEXTUnique(
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
    for (auto gpu : m_instance->enumeratePhysicalDevices()) {
        if (gpu.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            m_physicalDevice = gpu;
        }
    }
    // If discrete gpu not found, select first gpu
    if (!m_physicalDevice) {
        m_physicalDevice = m_instance->enumeratePhysicalDevices().front();
    }
    spdlog::info("Selected GPU: {}", std::string(m_physicalDevice.getProperties().deviceName.data()));

    // Find queue family
    spdlog::info("Selected queue families:");
    std::vector properties = m_physicalDevice.getQueueFamilyProperties();
    for (uint32_t index = 0; index < properties.size(); index++) {
        auto supportGraphics = properties[index].queueFlags & vk::QueueFlagBits::eGraphics;
        auto supportCompute = properties[index].queueFlags & vk::QueueFlagBits::eCompute;
        auto supportTransfer = properties[index].queueFlags & vk::QueueFlagBits::eTransfer;
        if (surface) {
            auto supportPresent = m_physicalDevice.getSurfaceSupportKHR(index, surface);
            if (supportGraphics && supportCompute && supportPresent && supportTransfer) {
                if (!m_queueFamilies.contains(QueueFlags::General)) {
                    m_queueFamilies[QueueFlags::General] = index;
                    spdlog::info("  General: {} x {}", index, properties[index].queueCount);
                    continue;
                }
            }
            if (supportGraphics && supportPresent) {
                if (!m_queueFamilies.contains(QueueFlags::Graphics)) {
                    m_queueFamilies[QueueFlags::Graphics] = index;
                    spdlog::info("  Graphics: {} x {}", index, properties[index].queueCount);
                    continue;
                }
            }
        } else {
            if (supportGraphics && supportCompute && supportTransfer) {
                if (!m_queueFamilies.contains(QueueFlags::General)) {
                    m_queueFamilies[QueueFlags::General] = index;
                    spdlog::info("  General: {} x {}", index, properties[index].queueCount);
                    continue;
                }
            }
            if (supportGraphics) {
                if (!m_queueFamilies.contains(QueueFlags::Graphics)) {
                    m_queueFamilies[QueueFlags::Graphics] = index;
                    spdlog::info("  Graphics: {} x {}", index, properties[index].queueCount);
                    continue;
                }
            }
        }

        // These are not related to surface
        if (supportCompute) {
            if (!m_queueFamilies.contains(QueueFlags::Compute)) {
                m_queueFamilies[QueueFlags::Compute] = index;
                spdlog::info("  Compute: {} x {}", index, properties[index].queueCount);
                continue;
            }
        }
        if (supportTransfer) {
            if (!m_queueFamilies.contains(QueueFlags::Transfer)) {
                m_queueFamilies[QueueFlags::Transfer] = index;
                spdlog::info("  Transfer: {} x {}", index, properties[index].queueCount);
            }
        }
    }

    if (!m_queueFamilies.contains(QueueFlags::General)) {
        throw std::runtime_error("Failed to find general queue family.");
    }
}

void Context::initDevice(const std::vector<const char*>& deviceExtensions,
                         const vk::PhysicalDeviceFeatures& deviceFeatures,
                         const void* deviceCreateInfoPNext,
                         bool enableRayTracing) {
    // Create device
    std::unordered_map<vk::QueueFlags, std::vector<float>> queuePriorities;
    std::vector<vk::DeviceQueueCreateInfo> queueInfo;
    for (const auto& [flag, queueFamily] : m_queueFamilies) {
        uint32_t queueCount = m_physicalDevice.getQueueFamilyProperties()[queueFamily].queueCount;
        queuePriorities[flag] = {};
        queuePriorities[flag].resize(queueCount);
        std::ranges::fill(queuePriorities[flag], 1.0f);

        vk::DeviceQueueCreateInfo info;
        info.setQueueFamilyIndex(queueFamily);
        info.setQueuePriorities(queuePriorities[flag]);
        queueInfo.push_back(info);
        m_queues[flag] = std::vector<ThreadQueue>(queueCount);
    }

    checkDeviceExtensionSupport(deviceExtensions);

    vk::DeviceCreateInfo deviceInfo;
    deviceInfo.setQueueCreateInfos(queueInfo);
    deviceInfo.setPEnabledExtensionNames(deviceExtensions);
    deviceInfo.setPEnabledFeatures(&deviceFeatures);
    deviceInfo.setPNext(deviceCreateInfoPNext);
    m_device = m_physicalDevice.createDeviceUnique(deviceInfo);

    spdlog::info("Enabled m_device extensions:");
    for (auto& extension : deviceExtensions) {
        spdlog::info("  {}", extension);
    }

    // Get queue and command pool
    for (const auto& [flag, queueFamily] : m_queueFamilies) {
        for (uint32_t i = 0; i < m_queues[flag].size(); i++) {
            m_queues[flag][i].queue = m_device->getQueue(queueFamily, i);

            vk::CommandPoolCreateInfo commandPoolCreateInfo;
            commandPoolCreateInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
            commandPoolCreateInfo.setQueueFamilyIndex(queueFamily);
            m_queues[flag][i].commandPool = m_device->createCommandPoolUnique(commandPoolCreateInfo);
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
    m_descriptorPool = m_device->createDescriptorPoolUnique(descriptorPoolCreateInfo);
}

auto Context::getQueue(vk::QueueFlags flag) const -> vk::Queue {
    return getThreadQueue(flag).queue;
}

auto Context::getQueueFamily(vk::QueueFlags flag) const -> uint32_t {
    return m_queueFamilies.at(flag);
}

auto Context::getCommandPool(vk::QueueFlags flag) const -> vk::CommandPool {
    return *getThreadQueue(flag).commandPool;
}

auto Context::allocateCommandBuffer(vk::QueueFlags flag) const -> CommandBufferHandle {
    vk::CommandPool commandPool = *getThreadQueue(flag).commandPool;
    vk::CommandBufferAllocateInfo commandBufferInfo;
    commandBufferInfo.setCommandPool(commandPool);
    commandBufferInfo.setLevel(vk::CommandBufferLevel::ePrimary);
    commandBufferInfo.setCommandBufferCount(1);

    vk::CommandBuffer commandBuffer = m_device->allocateCommandBuffers(commandBufferInfo).front();
    return std::make_shared<CommandBuffer>(*this, commandBuffer, commandPool, flag);
}

void Context::submit(CommandBufferHandle commandBuffer,
                     vk::PipelineStageFlags waitStage,
                     vk::Semaphore waitSemaphore,
                     vk::Semaphore signalSemaphore,
                     FenceHandle fence) const {
    vk::Queue queue = getThreadQueue(commandBuffer->getQueueFlags()).queue;

    vk::SubmitInfo submitInfo;
    submitInfo.setWaitDstStageMask(waitStage);
    submitInfo.setCommandBuffers(*commandBuffer->m_commandBuffer);
    submitInfo.setWaitSemaphores(waitSemaphore);
    submitInfo.setSignalSemaphores(signalSemaphore);

    queue.submit(submitInfo, fence ? fence->getFence() : nullptr);
}

void Context::submit(CommandBufferHandle commandBuffer, FenceHandle fence) const {
    vk::Queue queue = getThreadQueue(commandBuffer->getQueueFlags()).queue;

    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(*commandBuffer->m_commandBuffer);

    queue.submit(submitInfo, fence ? fence->getFence() : nullptr);
}

void Context::oneTimeSubmit(const std::function<void(CommandBufferHandle)>& command,
                            vk::QueueFlags flag) const {
    CommandBufferHandle commandBuffer = allocateCommandBuffer(flag);

    commandBuffer->begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    command(commandBuffer);
    commandBuffer->end();

    submit(commandBuffer);

    vk::Queue queue = getThreadQueue(commandBuffer->getQueueFlags()).queue;
    queue.waitIdle();
}

auto Context::findMemoryTypeIndex(vk::MemoryRequirements requirements,
                                  vk::MemoryPropertyFlags memoryProp) const -> uint32_t {
    vk::PhysicalDeviceMemoryProperties memProperties = m_physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i != memProperties.memoryTypeCount; ++i) {
        if ((requirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & memoryProp) == memoryProp) {
            return i;
        }
    }
    throw std::runtime_error("Failed to find m_memory m_type index.");
}

auto Context::getPhysicalDeviceLimits() const -> vk::PhysicalDeviceLimits {
    return m_physicalDevice.getProperties().limits;
}

auto Context::createShader(const ShaderCreateInfo& createInfo) const -> ShaderHandle {
    return std::make_shared<Shader>(*this, createInfo);
}

auto Context::createDescriptorSet(const DescriptorSetCreateInfo& createInfo) const
    -> DescriptorSetHandle {
    return std::make_shared<DescriptorSet>(*this, createInfo);
}

auto Context::createGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo) const
    -> GraphicsPipelineHandle {
    return std::make_shared<GraphicsPipeline>(*this, createInfo);
}

auto Context::createMeshShaderPipeline(const MeshShaderPipelineCreateInfo& createInfo) const
    -> MeshShaderPipelineHandle {
    return std::make_shared<MeshShaderPipeline>(*this, createInfo);
}

auto Context::createComputePipeline(const ComputePipelineCreateInfo& createInfo) const
    -> ComputePipelineHandle {
    return std::make_shared<ComputePipeline>(*this, createInfo);
}

auto Context::createRayTracingPipeline(const RayTracingPipelineCreateInfo& createInfo) const
    -> RayTracingPipelineHandle {
    return std::make_shared<RayTracingPipeline>(*this, createInfo);
}

auto Context::createImage(const ImageCreateInfo& createInfo) const -> ImageHandle {
    return std::make_shared<Image>(*this, createInfo);
}

auto Context::createBuffer(const BufferCreateInfo& createInfo) const -> BufferHandle {
    return std::make_shared<Buffer>(*this, createInfo);
}

auto Context::createBottomAccel(const BottomAccelCreateInfo& createInfo) const
    -> BottomAccelHandle {
    return std::make_shared<BottomAccel>(*this, createInfo);
}

auto Context::createTopAccel(const TopAccelCreateInfo& createInfo) const -> TopAccelHandle {
    return std::make_shared<TopAccel>(*this, createInfo);
}

auto Context::createGPUTimer(const GPUTimerCreateInfo& createInfo) const -> GPUTimerHandle {
    return std::make_shared<GPUTimer>(*this, createInfo);
}

auto Context::createFence(const FenceCreateInfo& createInfo) const -> FenceHandle {
    return std::make_shared<Fence>(*this, createInfo);
}

void Context::checkDeviceExtensionSupport(
    const std::vector<const char*>& requiredExtensions) const {
    std::vector<vk::ExtensionProperties> availableExtensions =
        m_physicalDevice.enumerateDeviceExtensionProperties();
    std::vector<std::string> requiredExtensionNames(requiredExtensions.begin(),
                                                    requiredExtensions.end());

    for (const auto& extension : availableExtensions) {
        std::erase(requiredExtensionNames, extension.extensionName);
    }

    if (!requiredExtensionNames.empty()) {
        std::stringstream message;
        message << "The following required extensions are not supported by the m_device:\n";
        for (const auto& name : requiredExtensionNames) {
            message << "\t" << name << "\n";
        }
        throw std::runtime_error(message.str());
    }
}

auto Context::getThreadQueue(vk::QueueFlags flag) const -> const ThreadQueue& {
    std::thread::id tid = std::this_thread::get_id();
    std::lock_guard<std::mutex> lock(m_queueMutex);

    // Find used queue
    auto& matchedQueues = m_queues.at(flag);
    for (auto& queue : matchedQueues) {
        if (queue.tid == tid) {
            return queue;
        }
    }

    // Use new queue
    for (auto& queue : matchedQueues) {
        if (queue.tid == std::thread::id{}) {
            std::ostringstream threadIdStream;
            threadIdStream << std::setw(5) << std::setfill(' ') << tid;
            std::string threadIdStr = threadIdStream.str();
            spdlog::debug("Use new queue: {}", threadIdStr);

            queue.tid = tid;
            return queue;
        }
    }

    // Not found
    throw std::runtime_error("Failed to get new queue.");
}
}  // namespace rv
