#include "Context.hpp"
#include <regex>
#include <spdlog/spdlog.h>
#include "Window/Window.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

void Context::Init()
{
    spdlog::info("Context::Init()");

    std::vector instanceExtensions = Window::GetExtensions();
    std::vector layers{ "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
    CreateInstance(instanceExtensions, layers);

    surface = Window::CreateSurface(*instance);

    CreateDevice();

    swapchain = Swapchain{ *device, *surface, queueFamily };
}

void Context::BeginRenderPass()
{
    swapchain.BeginRenderPass();
}

void Context::EndRenderPass()
{
    swapchain.EndRenderPass();
}

void Context::CopyToBackImage(vk::CommandBuffer commandBuffer, const Image& source)
{
    swapchain.CopyToBackImage(commandBuffer, source);
}
