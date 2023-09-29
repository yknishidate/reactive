#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <regex>
#include <vector>

#include <spdlog/spdlog.h>

#include <vulkan/vulkan.hpp>

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
    vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
    vk::BufferUsageFlagBits::eStorageBuffer |
    vk::BufferUsageFlagBits::eShaderDeviceAddress |
    vk::BufferUsageFlagBits::eVertexBuffer |
    vk::BufferUsageFlagBits::eTransferDst;
static constexpr vk::BufferUsageFlags Index =
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
};  // namespace MemoryUsage

namespace ImageUsage {
static vk::ImageUsageFlags ColorAttachment =
    vk::ImageUsageFlagBits::eColorAttachment |
    vk::ImageUsageFlagBits::eTransferSrc |
    vk::ImageUsageFlagBits::eTransferDst;
static vk::ImageUsageFlags DepthAttachment =
    vk::ImageUsageFlagBits::eDepthStencilAttachment |
    vk::ImageUsageFlagBits::eTransferDst;
static vk::ImageUsageFlags DepthStencilAttachment =
    vk::ImageUsageFlagBits::eDepthStencilAttachment |
    vk::ImageUsageFlagBits::eTransferDst;
static vk::ImageUsageFlags Storage =
    vk::ImageUsageFlagBits::eStorage |
    vk::ImageUsageFlagBits::eTransferDst |
    vk::ImageUsageFlagBits::eTransferSrc;
static vk::ImageUsageFlags Sampled =
    vk::ImageUsageFlagBits::eSampled |
    vk::ImageUsageFlagBits::eTransferDst |
    vk::ImageUsageFlagBits::eTransferSrc;
};  // namespace ImageUsage
// clang-format on

class Context {
    friend class CommandBuffer;

public:
    void initInstance(bool enableValidation,
                      const std::vector<const char*>& layers,
                      const std::vector<const char*>& instanceExtensions,
                      uint32_t apiVersion);

    void initPhysicalDevice(vk::SurfaceKHR surface = {});

    void initDevice(const std::vector<const char*>& deviceExtensions,
                    const vk::PhysicalDeviceFeatures& deviceFeatures,
                    const void* deviceCreateInfoPNext,
                    bool enableRayTracing);

    vk::Instance getInstance() const { return *instance; }
    vk::Device getDevice() const { return *device; }
    vk::PhysicalDevice getPhysicalDevice() const { return physicalDevice; }
    vk::Queue getQueue() const { return queue; }
    uint32_t getQueueFamily() const { return queueFamily; }
    vk::DescriptorPool getDescriptorPool() const { return *descriptorPool; }

    CommandBufferHandle allocateCommandBuffer() const;

    void submit(CommandBufferHandle commandBuffer, vk::Fence fence) const;

    void oneTimeSubmit(const std::function<void(CommandBufferHandle)>& command) const;

    vk::UniqueDescriptorSet allocateDescriptorSet(vk::DescriptorSetLayout descSetLayout) const;

    uint32_t findMemoryTypeIndex(vk::MemoryRequirements requirements,
                                 vk::MemoryPropertyFlags memoryProp) const;

    template <typename T>
    T getPhysicalDeviceProperties2() const {
        vk::PhysicalDeviceProperties2 props2;
        T props;
        props2.pNext = &props;
        physicalDevice.getProperties2(&props2);
        return props;
    }

    vk::PhysicalDeviceLimits getPhysicalDeviceLimits() const {
        vk::PhysicalDeviceProperties props = physicalDevice.getProperties();
        return props.limits;
    }

    bool debugEnabled() const { return debugMessenger.get(); }

    template <typename T, typename = std::enable_if_t<vk::isVulkanHandleType<T>::value>>
    void setDebugName(T object, const char* debugName) const {
        if (debugEnabled()) {
            vk::DebugUtilsObjectNameInfoEXT nameInfo;
            nameInfo.setObjectHandle(uint64_t(static_cast<T::CType>(object)));
            nameInfo.setObjectType(T::objectType);
            nameInfo.setPObjectName(debugName);
            getDevice().setDebugUtilsObjectNameEXT(nameInfo);
        }
    }

    ShaderHandle createShader(ShaderCreateInfo createInfo) const;
    DescriptorSetHandle createDescriptorSet(DescriptorSetCreateInfo createInfo) const;
    GraphicsPipelineHandle createGraphicsPipeline(GraphicsPipelineCreateInfo createInfo) const;
    MeshShaderPipelineHandle createMeshShaderPipeline(
        MeshShaderPipelineCreateInfo createInfo) const;
    ComputePipelineHandle createComputePipeline(ComputePipelineCreateInfo createInfo) const;
    RayTracingPipelineHandle createRayTracingPipeline(
        RayTracingPipelineCreateInfo createInfo) const;
    ImageHandle createImage(ImageCreateInfo createInfo) const;
    BufferHandle createBuffer(BufferCreateInfo createInfo) const;
    BottomAccelHandle createBottomAccel(BottomAccelCreateInfo createInfo) const;
    TopAccelHandle createTopAccel(TopAccelCreateInfo createInfo) const;
    GPUTimerHandle createGPUTimer(GPUTimerCreateInfo createInfo) const;

private:
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                  VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                  VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                  void* pUserData) {
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            spdlog::warn(pCallbackData->pMessage);
        } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            spdlog::error(pCallbackData->pMessage);
        }
        return VK_FALSE;
    }

    void checkDeviceExtensionSupport(const std::vector<const char*>& requiredExtensions) const {
        std::vector<vk::ExtensionProperties> availableExtensions =
            physicalDevice.enumerateDeviceExtensionProperties();
        std::vector<std::string> requiredExtensionNames(requiredExtensions.begin(),
                                                        requiredExtensions.end());

        for (const auto& extension : availableExtensions) {
            std::erase(requiredExtensionNames, extension.extensionName);
        }

        if (!requiredExtensionNames.empty()) {
            std::string message =
                "The following required extensions are not supported by the device:\n";
            for (const auto& name : requiredExtensionNames) {
                message.append("\t" + name + "\n");
            }
            throw std::runtime_error(message);
        }
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
}  // namespace rv
