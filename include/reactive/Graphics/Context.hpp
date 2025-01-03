#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <cstddef>
#include <map>
#include <regex>
#include <type_traits>
#include <vector>

#include <spdlog/spdlog.h>

#include <vulkan/vulkan.hpp>

namespace std {
template <>
struct hash<vk::QueueFlags> {
    std::size_t operator()(const vk::QueueFlags& flags) const {
        return std::hash<uint32_t>()(static_cast<uint32_t>(flags));
    }
};
}  // namespace std

namespace rv {
struct ImageCreateInfo;
struct BufferCreateInfo;
struct MeshCreateInfo;
struct SphereMeshCreateInfo;
struct PlaneMeshCreateInfo;
struct PlaneLineMeshCreateInfo;
struct CubeMeshCreateInfo;
struct CubeLineMeshCreateInfo;
struct ShaderCreateInfo;
struct DescriptorSetCreateInfo;
struct GraphicsPipelineCreateInfo;
struct MeshShaderPipelineCreateInfo;
struct ComputePipelineCreateInfo;
struct RayTracingPipelineCreateInfo;
struct BottomAccelCreateInfo;
struct TopAccelCreateInfo;
struct GPUTimerCreateInfo;
struct FenceCreateInfo;
class Buffer;
class Image;
class Mesh;
class Shader;
class DescriptorSet;
class Pipeline;
class GraphicsPipeline;
class MeshShaderPipeline;
class ComputePipeline;
class RayTracingPipeline;
class BottomAccel;
class TopAccel;
class GPUTimer;
class CommandBuffer;
class Fence;

using BufferHandle = std::shared_ptr<Buffer>;
using ImageHandle = std::shared_ptr<Image>;
using ShaderHandle = std::shared_ptr<Shader>;
using DescriptorSetHandle = std::shared_ptr<DescriptorSet>;
using PipelineHandle = std::shared_ptr<Pipeline>;
using GraphicsPipelineHandle = std::shared_ptr<GraphicsPipeline>;
using MeshShaderPipelineHandle = std::shared_ptr<MeshShaderPipeline>;
using ComputePipelineHandle = std::shared_ptr<ComputePipeline>;
using RayTracingPipelineHandle = std::shared_ptr<RayTracingPipeline>;
using BottomAccelHandle = std::shared_ptr<BottomAccel>;
using TopAccelHandle = std::shared_ptr<TopAccel>;
using GPUTimerHandle = std::shared_ptr<GPUTimer>;
using CommandBufferHandle = std::shared_ptr<CommandBuffer>;
using FenceHandle = std::shared_ptr<Fence>;

// clang-format off
namespace BufferUsage {
static constexpr vk::BufferUsageFlags Uniform =
    vk::BufferUsageFlagBits::eUniformBuffer |
    vk::BufferUsageFlagBits::eTransferDst |
    vk::BufferUsageFlagBits::eShaderDeviceAddress;
static constexpr vk::BufferUsageFlags Storage =
    vk::BufferUsageFlagBits::eStorageBuffer |
    vk::BufferUsageFlagBits::eTransferDst |
    vk::BufferUsageFlagBits::eShaderDeviceAddress;
static constexpr vk::BufferUsageFlags Staging =
    vk::BufferUsageFlagBits::eTransferSrc |
    vk::BufferUsageFlagBits::eTransferDst;
static constexpr vk::BufferUsageFlags Vertex =
    vk::BufferUsageFlagBits::eStorageBuffer |
    vk::BufferUsageFlagBits::eShaderDeviceAddress |
    vk::BufferUsageFlagBits::eVertexBuffer |
    vk::BufferUsageFlagBits::eTransferDst;
static constexpr vk::BufferUsageFlags Index =
    vk::BufferUsageFlagBits::eStorageBuffer |
    vk::BufferUsageFlagBits::eShaderDeviceAddress |
    vk::BufferUsageFlagBits::eIndexBuffer |
    vk::BufferUsageFlagBits::eTransferDst;
static constexpr vk::BufferUsageFlags AccelVertex =
    vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
    vk::BufferUsageFlagBits::eStorageBuffer |
    vk::BufferUsageFlagBits::eShaderDeviceAddress |
    vk::BufferUsageFlagBits::eVertexBuffer |
    vk::BufferUsageFlagBits::eTransferDst;
static constexpr vk::BufferUsageFlags AccelIndex =
    vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
    vk::BufferUsageFlagBits::eStorageBuffer |
    vk::BufferUsageFlagBits::eShaderDeviceAddress |
    vk::BufferUsageFlagBits::eIndexBuffer |
    vk::BufferUsageFlagBits::eTransferDst;
static constexpr vk::BufferUsageFlags Indirect =
    vk::BufferUsageFlagBits::eStorageBuffer |
    vk::BufferUsageFlagBits::eTransferDst |
    vk::BufferUsageFlagBits::eIndirectBuffer;
static constexpr vk::BufferUsageFlags AccelStorage =
    vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
    vk::BufferUsageFlagBits::eShaderDeviceAddress;
static constexpr vk::BufferUsageFlags AccelInput =
    vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
    vk::BufferUsageFlagBits::eStorageBuffer |
    vk::BufferUsageFlagBits::eShaderDeviceAddress |
    vk::BufferUsageFlagBits::eTransferDst;
static constexpr vk::BufferUsageFlags ShaderBindingTable =
    vk::BufferUsageFlagBits::eShaderBindingTableKHR |
    vk::BufferUsageFlagBits::eShaderDeviceAddress |
    vk::BufferUsageFlagBits::eTransferDst;
static constexpr vk::BufferUsageFlags Scratch =
    vk::BufferUsageFlagBits::eStorageBuffer |
    vk::BufferUsageFlagBits::eShaderDeviceAddress;
}  // namespace BufferUsage

namespace MemoryUsage {
static constexpr vk::MemoryPropertyFlags Device =
    vk::MemoryPropertyFlagBits::eDeviceLocal;
static constexpr vk::MemoryPropertyFlags Host =
    vk::MemoryPropertyFlagBits::eHostVisible |
    vk::MemoryPropertyFlagBits::eHostCoherent;
static constexpr vk::MemoryPropertyFlags DeviceHost =
    vk::MemoryPropertyFlagBits::eDeviceLocal |
    vk::MemoryPropertyFlagBits::eHostVisible |
    vk::MemoryPropertyFlagBits::eHostCoherent;
}  // namespace MemoryUsage

namespace ImageUsage {
static constexpr vk::ImageUsageFlags ColorAttachment =
    vk::ImageUsageFlagBits::eColorAttachment |
    vk::ImageUsageFlagBits::eTransferSrc |
    vk::ImageUsageFlagBits::eTransferDst;
static constexpr vk::ImageUsageFlags DepthAttachment =
    vk::ImageUsageFlagBits::eDepthStencilAttachment |
    vk::ImageUsageFlagBits::eTransferDst;
static constexpr vk::ImageUsageFlags DepthStencilAttachment =
    vk::ImageUsageFlagBits::eDepthStencilAttachment |
    vk::ImageUsageFlagBits::eTransferDst;
static constexpr vk::ImageUsageFlags Storage =
    vk::ImageUsageFlagBits::eStorage |
    vk::ImageUsageFlagBits::eTransferDst |
    vk::ImageUsageFlagBits::eTransferSrc;
static constexpr vk::ImageUsageFlags Sampled =
    vk::ImageUsageFlagBits::eSampled |
    vk::ImageUsageFlagBits::eTransferDst |
    vk::ImageUsageFlagBits::eTransferSrc;
}  // namespace ImageUsage

namespace QueueFlags{
    static constexpr vk::QueueFlags General =
        vk::QueueFlagBits::eGraphics |
        vk::QueueFlagBits::eCompute |
        vk::QueueFlagBits::eTransfer;
    static constexpr vk::QueueFlags Graphics = 
        vk::QueueFlagBits::eGraphics;
    static constexpr vk::QueueFlags Compute =  
        vk::QueueFlagBits::eCompute;
    static constexpr vk::QueueFlags Transfer = 
        vk::QueueFlagBits::eTransfer;
}
// clang-format on

class Context {
    friend class CommandBuffer;

public:
    // Initialization
    void initInstance(bool enableValidation,
                      const std::vector<const char*>& layers,
                      const std::vector<const char*>& instanceExtensions,
                      uint32_t apiVersion);

    void initPhysicalDevice(vk::SurfaceKHR surface = {});

    void initDevice(const std::vector<const char*>& deviceExtensions,
                    const vk::PhysicalDeviceFeatures& deviceFeatures,
                    const void* deviceCreateInfoPNext,
                    bool enableRayTracing);

    // Getter
    auto getInstance() const -> vk::Instance { return *m_instance; }

    auto getDevice() const -> vk::Device { return *m_device; }

    auto getPhysicalDevice() const -> vk::PhysicalDevice { return m_physicalDevice; }

    auto getQueue(vk::QueueFlags flag = QueueFlags::General) const -> vk::Queue;

    auto getQueueFamily(vk::QueueFlags flag = QueueFlags::General) const -> uint32_t;

    auto getCommandPool(vk::QueueFlags flag = QueueFlags::General) const -> vk::CommandPool;

    auto getDescriptorPool() const -> vk::DescriptorPool { return *m_descriptorPool; }

    // Command buffer
    auto allocateCommandBuffer(vk::QueueFlags flag = QueueFlags::General) const
        -> CommandBufferHandle;

    void submit(CommandBufferHandle commandBuffer,
                vk::PipelineStageFlags waitStage,
                vk::Semaphore waitSemaphore,
                vk::Semaphore signalSemaphore,
                FenceHandle fence = {}) const;

    void submit(CommandBufferHandle commandBuffer, FenceHandle fence = {}) const;

    void oneTimeSubmit(const std::function<void(CommandBufferHandle)>& command,
                       vk::QueueFlags flag = QueueFlags::General) const;

    // Memory
    auto findMemoryTypeIndex(vk::MemoryRequirements requirements,
                             vk::MemoryPropertyFlags memoryProp) const -> uint32_t;

    // Physical device
    template <typename T>
    auto getPhysicalDeviceProperties2() const -> T {
        vk::PhysicalDeviceProperties2 props2;
        T props;
        props2.pNext = &props;
        m_physicalDevice.getProperties2(&props2);
        return props;
    }

    auto getPhysicalDeviceLimits() const -> vk::PhysicalDeviceLimits;

    // Debug
    auto debugEnabled() const -> bool { return m_debugMessenger.get(); }

    template <typename T, typename = std::enable_if_t<vk::isVulkanHandleType<T>::value>>
    void setDebugName(T object, const char* debugName) const {
        if (debugEnabled()) {
            vk::DebugUtilsObjectNameInfoEXT nameInfo;
            nameInfo.setObjectHandle(uint64_t(static_cast<typename T::CType>(object)));
            nameInfo.setObjectType(T::objectType);
            nameInfo.setPObjectName(debugName);
            getDevice().setDebugUtilsObjectNameEXT(nameInfo);
        }
    }

    // Resource
    auto createShader(const ShaderCreateInfo& createInfo) const -> ShaderHandle;

    auto createDescriptorSet(const DescriptorSetCreateInfo& createInfo) const
        -> DescriptorSetHandle;

    auto createGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo) const
        -> GraphicsPipelineHandle;

    auto createMeshShaderPipeline(const MeshShaderPipelineCreateInfo& createInfo) const
        -> MeshShaderPipelineHandle;

    auto createComputePipeline(const ComputePipelineCreateInfo& createInfo) const
        -> ComputePipelineHandle;

    auto createRayTracingPipeline(const RayTracingPipelineCreateInfo& createInfo) const
        -> RayTracingPipelineHandle;

    auto createImage(const ImageCreateInfo& createInfo) const -> ImageHandle;

    auto createBuffer(const BufferCreateInfo& createInfo) const -> BufferHandle;

    auto createBottomAccel(const BottomAccelCreateInfo& createInfo) const -> BottomAccelHandle;

    auto createTopAccel(const TopAccelCreateInfo& createInfo) const -> TopAccelHandle;

    auto createGPUTimer(const GPUTimerCreateInfo& createInfo) const -> GPUTimerHandle;

    auto createFence(const FenceCreateInfo& createInfo) const -> FenceHandle;

private:
    static auto VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                         VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                         VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                                         void* pUserData) -> VkBool32 {
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            spdlog::warn(pCallbackData->pMessage);
        } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            spdlog::error(pCallbackData->pMessage);
        }
        return VK_FALSE;
    }

    void checkDeviceExtensionSupport(const std::vector<const char*>& requiredExtensions) const;

    struct ThreadQueue {
        std::thread::id tid{};
        vk::Queue queue{};
        vk::UniqueCommandPool commandPool;
    };
    auto getThreadQueue(vk::QueueFlags flag) const -> const ThreadQueue&;

    vk::UniqueInstance m_instance;
    vk::UniqueDebugUtilsMessengerEXT m_debugMessenger;
    vk::UniqueDevice m_device;
    vk::PhysicalDevice m_physicalDevice;

    mutable std::mutex m_queueMutex;
    mutable std::map<vk::QueueFlags, std::vector<ThreadQueue>> m_queues;
    std::unordered_map<vk::QueueFlags, uint32_t> m_queueFamilies;
    vk::UniqueDescriptorPool m_descriptorPool;
};
}  // namespace rv
