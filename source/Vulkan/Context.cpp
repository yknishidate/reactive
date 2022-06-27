#include "Context.hpp"
#include <iostream>
#include <regex>
#include <spdlog/spdlog.h>
#include "Window.hpp"
#include "Helper.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

void Context::Init()
{
    spdlog::info("Context::Init()");

    std::vector layers{ "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };

    instance = Helper::CreateInstance(Window::GetExtensions(), layers);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

    debugMessenger = Helper::CreateDebugMessenger(*instance);

    physicalDevice = instance->enumeratePhysicalDevices().front();

    surface = Window::CreateSurface(*instance);

    queueFamily = Helper::FindGenericQueueFamily(physicalDevice, *surface);

    device = Helper::CreateDevice(physicalDevice, queueFamily);

    queue = device->getQueue(queueFamily, 0);

    commandPool = Helper::CreateCommandPool(*device, queueFamily);

    descriptorPool = Helper::CraeteDescriptorPool(*device);

    swapchain = Swapchain{ *device, *surface, queueFamily };
}

vk::UniqueCommandBuffer Context::AllocateCommandBuffer()
{
    return std::move(Helper::AllocateCommandBuffers(*device, *commandPool, 1).front());
}

void Context::SubmitAndWait(vk::CommandBuffer commandBuffer)
{
    queue.submit(
        vk::SubmitInfo()
        .setCommandBuffers(commandBuffer)
    );
    queue.waitIdle();
}

void Context::OneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command)
{
    vk::UniqueCommandBuffer commandBuffer = AllocateCommandBuffer();

    commandBuffer->begin(
        vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
    );
    command(*commandBuffer);
    commandBuffer->end();

    SubmitAndWait(*commandBuffer);
}

uint32_t Context::FindMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags memoryProp)
{
    const vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();
    for (uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index) {
        auto propertyFlags = memoryProperties.memoryTypes[index].propertyFlags;
        bool match = (propertyFlags & memoryProp) == memoryProp;
        if (requirements.memoryTypeBits & (1 << index) && match) {
            return index;
        }
    }
    throw std::runtime_error("Failed to find memory type index.");
}

void Context::BeginRenderPass()
{
    swapchain.BeginRenderPass();
}

void Context::EndRenderPass()
{
    swapchain.EndRenderPass();
}

void Context::WaitNextFrame()
{
    swapchain.WaitNextFrame(*device);
}

vk::CommandBuffer Context::BeginCommandBuffer()
{
    return swapchain.BeginCommandBuffer();
}

void Context::Submit()
{
    swapchain.Submit(queue);
}

void Context::Present()
{
    swapchain.Present(queue);
}
