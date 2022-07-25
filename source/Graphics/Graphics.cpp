#include "Graphics.hpp"
#include <regex>
#include <spdlog/spdlog.h>
#include "Window/Window.hpp"
#include "Helper.hpp"
#include "Buffer.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

void Graphics::Init()
{
    spdlog::info("Graphics::Init()");

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

    descriptorPool = Helper::CreateDescriptorPool(*device);

    swapchain = Swapchain{ *device, *surface, queueFamily };
}

vk::UniqueCommandBuffer Graphics::AllocateCommandBuffer()
{
    return std::move(Helper::AllocateCommandBuffers(*device, *commandPool, 1).front());
}

void Graphics::SubmitAndWait(vk::CommandBuffer commandBuffer)
{
    queue.submit(
        vk::SubmitInfo()
        .setCommandBuffers(commandBuffer)
    );
    queue.waitIdle();
}

void Graphics::OneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command)
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

uint32_t Graphics::FindMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags memoryProp)
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
