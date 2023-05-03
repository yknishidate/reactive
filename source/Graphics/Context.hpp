#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <vector>

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

struct BufferCreateInfo;
struct MeshCreateInfo;
struct SphereMeshCreateInfo;
struct PlaneMeshCreateInfo;
struct CubeMeshCreateInfo;
struct ShaderCreateInfo;
struct DescriptorSetCreateInfo;
struct GraphicsPipelineCreateInfo;
class HostBuffer;
class DeviceBuffer;
class Mesh;
class Shader;
class DescriptorSet;
class GraphicsPipeline;

class Context {
public:
    void initInstance(bool enableValidation,
                      const std::vector<const char*>& layers,
                      const std::vector<const char*>& instanceExtensions,
                      uint32_t apiVersion);

    void initPhysicalDevice(vk::SurfaceKHR surface);

    void initDevice(const std::vector<const char*>& deviceExtensions,
                    const vk::PhysicalDeviceFeatures& deviceFeatures,
                    const void* deviceCreateInfoPNext);

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

    Shader createShader(ShaderCreateInfo createInfo) const;
    DescriptorSet createDescriptorSet(DescriptorSetCreateInfo createInfo) const;
    GraphicsPipeline createGraphicsPipeline(GraphicsPipelineCreateInfo createInfo) const;
    HostBuffer createHostBuffer(BufferCreateInfo createInfo) const;
    DeviceBuffer createDeviceBuffer(BufferCreateInfo createInfo) const;
    Mesh createMesh(MeshCreateInfo createInfo) const;
    Mesh createSphereMesh(SphereMeshCreateInfo createInfo) const;
    Mesh createPlaneMesh(PlaneMeshCreateInfo createInfo) const;
    Mesh createCubeMesh(CubeMeshCreateInfo createInfo) const;

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
