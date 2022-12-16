#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <spdlog/spdlog.h>
#include <regex>
#include <functional>
#include "Swapchain.hpp"

struct Context
{
    static void Init();

    static vk::UniqueCommandBuffer AllocateCommandBuffer();

    static void SubmitAndWait(vk::CommandBuffer commandBuffer);

    static void OneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command);

    static uint32_t FindMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags memoryProp);

    static auto GetInstance() { return *instance; }
    static auto GetDevice() { return *device; }
    static auto GetPhysicalDevice() { return physicalDevice; }
    static auto GetQueueFamily() { return queueFamily; }
    static auto GetQueue() { return queue; }
    static auto GetDescriptorPool() { return *descriptorPool; }
    static auto GetImageCount() { return swapchain.GetImageCount(); }
    static auto GetMinImageCount() { return swapchain.GetMinImageCount(); }
    static auto GetRenderPass() { return swapchain.GetRenderPass(); }
    static const auto& GetSwapchain() { return swapchain; }

    static void BeginRenderPass();
    static void EndRenderPass();
    static void CopyToBackImage(vk::CommandBuffer commandBuffer, const Image& source);
    static void WaitNextFrame() { swapchain.WaitNextFrame(*device); }
    static auto BeginCommandBuffer() -> vk::CommandBuffer { return swapchain.BeginCommandBuffer(); }
    static void Submit() { swapchain.Submit(queue); }
    static void Present() { swapchain.Present(queue); }

private:
    static VKAPI_ATTR VkBool32 VKAPI_CALL
        DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
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
