#include "Graphics.hpp"
#include <regex>
#include <spdlog/spdlog.h>
#include "Window/Window.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

void Graphics::Init()
{
	spdlog::info("Graphics::Init()");

	std::vector instanceExtensions = Window::GetExtensions();
	std::vector layers{ "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
	instance = Instance::Create(instanceExtensions, layers);

	surface = Window::CreateSurface(instance.GetInstance());

	device = instance.CreateDevice(*surface);

	swapchain = device.CreateSwapchain(*surface);
}

void Graphics::BeginRenderPass()
{
	swapchain.BeginRenderPass();
}

void Graphics::EndRenderPass()
{
	swapchain.EndRenderPass();
}

void Graphics::CopyToBackImage(vk::CommandBuffer commandBuffer, const Image& source)
{
	swapchain.CopyToBackImage(commandBuffer, source);
}
