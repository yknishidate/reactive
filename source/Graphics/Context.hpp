#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <spdlog/spdlog.h>
#include <functional>
#include <regex>
#include <vulkan/vulkan.hpp>
#include "Swapchain.hpp"

class Context {
public:
    static void init(bool enableValidation);

    static std::vector<vk::UniqueCommandBuffer> allocateCommandBuffers(uint32_t count);
    static vk::UniqueCommandBuffer allocateCommandBuffer();

    static void oneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command);

    static uint32_t findMemoryTypeIndex(vk::MemoryRequirements requirements,
                                        vk::MemoryPropertyFlags memoryProp);

    static auto getInstance() { return *instance; }
    static auto getDevice() { return *device; }
    static auto getPhysicalDevice() { return physicalDevice; }
    static auto getQueueFamily() { return queueFamily; }
    static auto getQueue() { return queue; }
    static auto getDescriptorPool() { return *descriptorPool; }
    static auto getSurface() { return *surface; }

    // timestamp
    static void beginTimestamp(vk::CommandBuffer commandBuffer, uint32_t queryIndex);
    static void endTimestamp(vk::CommandBuffer commandBuffer, uint32_t queryIndex);
    static float getElapsedTimeNS(uint32_t queryIndex);

    static void waitIdle() { device->waitIdle(); }

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

    static inline vk::UniqueInstance instance;
    static inline vk::UniqueDebugUtilsMessengerEXT debugMessenger;
    static inline vk::UniqueSurfaceKHR surface;
    static inline vk::UniqueDevice device;
    static inline vk::PhysicalDevice physicalDevice;
    static inline uint32_t queueFamily = ~0u;
    static inline vk::Queue queue;
    static inline vk::UniqueCommandPool commandPool;
    static inline vk::UniqueDescriptorPool descriptorPool;
    static inline vk::UniqueQueryPool queryPool;
    static inline float timestampPeriod;
    static inline std::array<uint64_t, 6> timestamps{};  // 3 framebuffers * 2(start/end)
};
