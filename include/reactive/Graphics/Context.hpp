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
class Image;
class HostBuffer;
class DeviceBuffer;
class Mesh;
class Shader;
class DescriptorSet;
class GraphicsPipeline;
class MeshShaderPipeline;
class ComputePipeline;
class RayTracingPipeline;
class BottomAccel;
class TopAccel;
class GPUTimer;

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

    Shader createShader(ShaderCreateInfo createInfo) const;
    DescriptorSet createDescriptorSet(DescriptorSetCreateInfo createInfo) const;
    GraphicsPipeline createGraphicsPipeline(GraphicsPipelineCreateInfo createInfo) const;
    MeshShaderPipeline createMeshShaderPipeline(MeshShaderPipelineCreateInfo createInfo) const;
    ComputePipeline createComputePipeline(ComputePipelineCreateInfo createInfo) const;
    RayTracingPipeline createRayTracingPipeline(RayTracingPipelineCreateInfo createInfo) const;
    Image createImage(ImageCreateInfo createInfo) const;
    HostBuffer createHostBuffer(BufferCreateInfo createInfo) const;
    DeviceBuffer createDeviceBuffer(BufferCreateInfo createInfo) const;
    Mesh createMesh(MeshCreateInfo createInfo) const;
    Mesh createSphereMesh(SphereMeshCreateInfo createInfo) const;
    Mesh createPlaneMesh(PlaneMeshCreateInfo createInfo) const;
    Mesh createCubeMesh(CubeMeshCreateInfo createInfo) const;
    Mesh createCubeLineMesh(CubeLineMeshCreateInfo createInfo) const;
    BottomAccel createBottomAccel(BottomAccelCreateInfo createInfo) const;
    TopAccel createTopAccel(TopAccelCreateInfo createInfo) const;
    GPUTimer createGPUTimer(GPUTimerCreateInfo createInfo) const;

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
