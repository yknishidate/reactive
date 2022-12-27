#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <spdlog/spdlog.h>
#include <regex>
#include <functional>
#include "Swapchain.hpp"

struct Context
{
    static void init();

    static std::vector<vk::UniqueCommandBuffer> allocateCommandBuffers(uint32_t count);
    static vk::UniqueCommandBuffer allocateCommandBuffer();

    static void oneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command);

    static uint32_t findMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags memoryProp);

    static auto getInstance() { return *instance; }
    static auto getDevice() { return *device; }
    static auto getPhysicalDevice() { return physicalDevice; }
    static auto getQueueFamily() { return queueFamily; }
    static auto getQueue() { return queue; }
    static auto getDescriptorPool() { return *descriptorPool; }
    static auto getImageCount() { return swapchain.getImageCount(); }
    static auto getMinImageCount() { return swapchain.getMinImageCount(); }
    static auto getRenderPass() { return swapchain.getRenderPass(); }
    static const auto& getSwapchain() { return swapchain; }

    static void beginRenderPass();
    static void endRenderPass();
    static void copyToBackImage(vk::CommandBuffer commandBuffer, const Image& source);
    static void waitNextFrame() { swapchain.waitNextFrame(*device); }
    static auto beginCommandBuffer() -> vk::CommandBuffer { return swapchain.beginCommandBuffer(); }
    static void submit() { swapchain.submit(queue); }
    static void present() { swapchain.present(queue); }

    static void clearBackImage(vk::CommandBuffer commandBuffer, std::array<float, 4> color);

private:
    static VKAPI_ATTR VkBool32 VKAPI_CALL
        debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                      VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                      VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                      void* pUserData) {
        const std::string message{ pCallbackData->pMessage };
        const std::regex regex{ "The Vulkan spec states: " };
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
    static inline uint32_t queueFamily = std::numeric_limits<uint32_t>::max();
    static inline vk::Queue queue;
    static inline vk::UniqueCommandPool commandPool;
    static inline vk::UniqueDescriptorPool descriptorPool;
    static inline Swapchain swapchain;
};
