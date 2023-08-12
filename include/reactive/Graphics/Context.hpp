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

using BufferHandle = std::shared_ptr<Buffer>;
using ImageHandle = std::shared_ptr<Image>;
using MeshHandle = std::shared_ptr<Mesh>;
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

enum class BufferUsage {
    Uniform,
    Storage,
    Staging,
    Vertex,
    Index,
    Indirect,
    AccelStorage,
    AccelInput,
    ShaderBindingTable,
    Scratch,
};

enum class MemoryUsage {
    Device,
    Host,
};

struct BufferCreateInfo {
    BufferUsage usage;
    MemoryUsage memory;
    size_t size = 0;
    const void* data = nullptr;
};

enum class ImageUsage {
    ColorAttachment,
    DepthAttachment,
    DepthStencilAttachment,
    Storage,
    Sampled,
};

enum class ImageLayout {
    Undefined,
    General,
    ColorAttachment,
    DepthAttachment,
    StencilAttachment,
    DepthStencilAttachment,
    ShaderReadOnly,
    TransferSrc,
    TransferDst,
    PresentSrc,
};

enum class Format {
    BGRA8Unorm,
    RGBA8Unorm,
    RGB16Sfloat,
    RGB32Sfloat,
    RGBA32Sfloat,
    D32Sfloat,
};

struct ImageCreateInfo {
    ImageUsage usage;
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    Format format;
    ImageLayout layout;
    // if mipLevels is std::numeric_limits<uint32_t>::max(), then it's set to max level
    uint32_t mipLevels = 1;
};

vk::BufferUsageFlags getBufferUsage(rv::BufferUsage usage);

vk::MemoryPropertyFlags getMemoryProperty(rv::MemoryUsage usage);

vk::ImageUsageFlags getImageUsage(rv::ImageUsage usage);

vk::ImageAspectFlags getImageAspect(rv::ImageUsage usage);

vk::ImageLayout getImageLayout(rv::ImageLayout layout);

vk::Format getFormat(rv::Format format);

class Context {
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

    std::vector<vk::UniqueCommandBuffer> allocateCommandBuffers(uint32_t count) const;

    void oneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command) const;

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
    MeshHandle createMesh(MeshCreateInfo createInfo) const;
    MeshHandle createSphereMesh(SphereMeshCreateInfo createInfo) const;
    MeshHandle createPlaneMesh(PlaneMeshCreateInfo createInfo) const;
    MeshHandle createCubeMesh(CubeMeshCreateInfo createInfo) const;
    MeshHandle createCubeLineMesh(CubeLineMeshCreateInfo createInfo) const;
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
