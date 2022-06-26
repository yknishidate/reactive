#pragma once
#include <regex>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <spdlog/spdlog.h>

namespace Helper
{
    VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                                                void* pUserData);

    vk::UniqueInstance CreateInstance(const std::vector<const char*>& extensions,
                                      const std::vector<const char*>& layers);

    vk::UniqueDebugUtilsMessengerEXT CreateDebugMessenger(vk::Instance instance);

    uint32_t FindQueueFamily(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);

    vk::UniqueDevice CreateDevice(vk::PhysicalDevice physicalDevice, uint32_t queueFamily);

    vk::UniqueCommandPool CreateCommandPool(vk::Device device, uint32_t queueFamily);

    vk::UniqueDescriptorPool CraeteDescriptorPool(vk::Device device);

    vk::UniqueSwapchainKHR CreateSwapchain(vk::Device device, vk::SurfaceKHR surface, uint32_t minImageCount, uint32_t queueFamily);

    std::vector<vk::UniqueImageView> CreateImageViews(vk::Device device, const std::vector<vk::Image>& images);

    vk::UniqueRenderPass CreateRenderPass(vk::Device device);
}
