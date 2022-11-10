#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <regex>
#include <spdlog/spdlog.h>
#include "Device.hpp"

struct Instance {
	auto GetInstance() { return *instance; }

	Device CreateDevice(vk::SurfaceKHR surface = VK_NULL_HANDLE)
	{
		return { *instance, surface };
	}

	static Instance Create(const std::vector<const char*>& extensions, const std::vector<const char*>& layers) {
		static const vk::DynamicLoader dl;
		const auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
		VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

		// Create instance
		const auto appInfo = vk::ApplicationInfo().setApiVersion(VK_API_VERSION_1_3);

		Instance instance;
		instance.instance = vk::createInstanceUnique(vk::InstanceCreateInfo()
			.setPApplicationInfo(&appInfo)
			.setPEnabledExtensionNames(extensions)
			.setPEnabledLayerNames(layers));
		VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance.instance);

		// Create debug messenger
		instance.debugMessenger = instance.instance->createDebugUtilsMessengerEXTUnique(
			vk::DebugUtilsMessengerCreateInfoEXT()
			.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
			.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
			.setPfnUserCallback(&DebugCallback));

		return instance;
	}

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

	vk::UniqueInstance instance;
	vk::UniqueDebugUtilsMessengerEXT debugMessenger;
	vk::UniqueSurfaceKHR surface;
};
