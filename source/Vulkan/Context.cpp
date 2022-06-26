#include "Context.hpp"
#include <iostream>
#include <regex>
#include <spdlog/spdlog.h>
#include "Window.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace
{
    VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                                                void* pUserData)
    {
        const std::string message{ pCallbackData->pMessage };
        const std::regex regex{ "The Vulkan spec states: " };
        std::smatch result;
        if (std::regex_search(message, result, regex)) {
            spdlog::error("{}\n {}\n", message.substr(0, result.position()), message.substr(result.position()));
        } else {
            spdlog::error("{}\n", message);
        }
        //assert(false);
        return VK_FALSE;
    }

    vk::UniqueInstance CreateInstance(const std::vector<const char*>& extensions,
                                      const std::vector<const char*>& layers)
    {
        static const vk::DynamicLoader dl;
        const auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        const auto appInfo = vk::ApplicationInfo()
            .setApiVersion(VK_API_VERSION_1_2);

        return vk::createInstanceUnique(
            vk::InstanceCreateInfo()
            .setPApplicationInfo(&appInfo)
            .setPEnabledExtensionNames(extensions)
            .setPEnabledLayerNames(layers)
        );
    }

    vk::UniqueDebugUtilsMessengerEXT CreateDebugMessenger(vk::Instance instance)
    {
        return instance.createDebugUtilsMessengerEXTUnique(
            vk::DebugUtilsMessengerCreateInfoEXT()
            .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
            .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
            .setPfnUserCallback(&DebugMessage)
        );
    }

    uint32_t FindQueueFamily(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
    {
        const std::vector queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
        for (uint32_t index = 0; index < queueFamilyProperties.size(); index++) {
            const auto supportGraphics = queueFamilyProperties[index].queueFlags & vk::QueueFlagBits::eGraphics;
            const auto supportCompute = queueFamilyProperties[index].queueFlags & vk::QueueFlagBits::eCompute;
            const auto supportPresent = physicalDevice.getSurfaceSupportKHR(index, surface);
            if (supportGraphics && supportCompute && supportPresent) {
                return index;
            }
        }
        throw std::runtime_error("Failed to find queue family.");
    }

    vk::UniqueDevice CreateDevice(vk::PhysicalDevice physicalDevice, uint32_t queueFamily)
    {
        const float queuePriority = 0.0f;
        const auto queueInfo = vk::DeviceQueueCreateInfo()
            .setQueueFamilyIndex(queueFamily)
            .setQueueCount(1)
            .setPQueuePriorities(&queuePriority);

        const std::vector deviceExtensions{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
            VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
            VK_KHR_MAINTENANCE3_EXTENSION_NAME,
            VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
        };

        const auto deviceFeatures = vk::PhysicalDeviceFeatures()
            .setShaderInt64(true)
            .setFragmentStoresAndAtomics(true)
            .setVertexPipelineStoresAndAtomics(true);

        const auto descFeatures = vk::PhysicalDeviceDescriptorIndexingFeatures()
            .setRuntimeDescriptorArray(true);

        const auto deviceInfo = vk::DeviceCreateInfo()
            .setQueueCreateInfos(queueInfo)
            .setPEnabledExtensionNames(deviceExtensions)
            .setPEnabledFeatures(&deviceFeatures);

        vk::StructureChain<vk::DeviceCreateInfo,
            vk::PhysicalDeviceBufferDeviceAddressFeatures,
            vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
            vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
            vk::PhysicalDeviceDescriptorIndexingFeatures>
            createInfoChain{ deviceInfo, {true}, {true}, {true}, descFeatures };

        return physicalDevice.createDeviceUnique(createInfoChain.get<vk::DeviceCreateInfo>());
    }

    vk::UniqueSwapchainKHR CreateSwapchain(vk::Device device, vk::SurfaceKHR surface, uint32_t minImageCount, uint32_t queueFamily)
    {
        return device.createSwapchainKHRUnique(
            vk::SwapchainCreateInfoKHR()
            .setSurface(surface)
            .setMinImageCount(minImageCount)
            .setImageFormat(vk::Format::eB8G8R8A8Unorm)
            .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
            .setImageExtent({ static_cast<uint32_t>(Window::GetWidth()), static_cast<uint32_t>(Window::GetHeight()) })
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
            .setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
            .setPresentMode(vk::PresentModeKHR::eFifo)
            .setClipped(true)
            .setQueueFamilyIndices(queueFamily)
        );
    }

    std::vector<vk::UniqueImageView> CreateImageViews(vk::Device device, const std::vector<vk::Image>& swapchainImages)
    {
        std::vector<vk::UniqueImageView> views;
        for (uint32_t i = 0; i < swapchainImages.size(); i++) {
            const auto viewInfo = vk::ImageViewCreateInfo()
                .setImage(swapchainImages[i])
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(vk::Format::eB8G8R8A8Unorm)
                .setComponents({ vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA })
                .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

            views.push_back(device.createImageViewUnique(viewInfo));
        }
        return views;
    }

    vk::UniqueCommandPool CreateCommandPool(vk::Device device, uint32_t queueFamily)
    {
        return device.createCommandPoolUnique(
            vk::CommandPoolCreateInfo()
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
            .setQueueFamilyIndex(queueFamily)
        );
    }

    vk::UniqueDescriptorPool CraeteDescriptorPool(vk::Device device)
    {
        const std::vector<vk::DescriptorPoolSize> poolSizes{
            { vk::DescriptorType::eSampler, 100 },
            { vk::DescriptorType::eCombinedImageSampler, 100 },
            { vk::DescriptorType::eSampledImage, 100 },
            { vk::DescriptorType::eStorageImage, 100 },
            { vk::DescriptorType::eUniformBuffer, 100 },
            { vk::DescriptorType::eStorageBuffer, 100 },
            { vk::DescriptorType::eInputAttachment, 100 }
        };

        return device.createDescriptorPoolUnique(
            vk::DescriptorPoolCreateInfo()
            .setPoolSizes(poolSizes)
            .setMaxSets(100)
            .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
        );
    }

    std::vector<const char*> GetExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        return extensions;
    }

    vk::UniqueSurfaceKHR CreateSurface(vk::Instance instance)
    {
        VkSurfaceKHR _surface;
        if (glfwCreateWindowSurface(VkInstance{ instance }, Window::GetWindow(), nullptr, &_surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
        return vk::UniqueSurfaceKHR{ _surface, {instance} };
    }

    vk::UniqueRenderPass CreateRenderPass(vk::Device device)
    {
        const auto attachment = vk::AttachmentDescription()
            .setFormat(vk::Format::eB8G8R8A8Unorm)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

        const auto colorAttachment = vk::AttachmentReference()
            .setAttachment(0)
            .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

        const auto subpass = vk::SubpassDescription()
            .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            .setColorAttachments(colorAttachment);

        const auto dependency = vk::SubpassDependency()
            .setSrcSubpass(VK_SUBPASS_EXTERNAL)
            .setDstSubpass(0)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

        return device.createRenderPassUnique(
            vk::RenderPassCreateInfo()
            .setAttachments(attachment)
            .setSubpasses(subpass)
            .setDependencies(dependency)
        );
    }
}

void Context::Init()
{
    spdlog::info("Context::Init()");
    std::vector layers{ "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
    instance = CreateInstance(GetExtensions(), layers);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);
    debugMessenger = CreateDebugMessenger(*instance);
    physicalDevice = instance->enumeratePhysicalDevices().front();
    surface = CreateSurface(*instance);
    queueFamily = FindQueueFamily(physicalDevice, *surface);
    device = CreateDevice(physicalDevice, queueFamily);
    queue = device->getQueue(queueFamily, 0);
    commandPool = CreateCommandPool(*device, queueFamily);
    swapchain = CreateSwapchain(*device, *surface, minImageCount, queueFamily);
    swapchainImages = device->getSwapchainImagesKHR(*swapchain);
    swapchainImageViews = CreateImageViews(*device, swapchainImages);
    descriptorPool = CraeteDescriptorPool(*device);
    renderPass = CreateRenderPass(*device);

    // Create Command Buffers
    size_t imageCount = swapchainImages.size();
    frames.resize(imageCount);
    frameSemaphores.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++) {
        {
            frames[i].framebuffer = device->createFramebufferUnique(
                vk::FramebufferCreateInfo()
                .setRenderPass(*renderPass)
                .setAttachments(*swapchainImageViews[i])
                .setWidth(Window::GetWidth())
                .setHeight(Window::GetHeight())
                .setLayers(1)
            );
        }
        {
            frames[i].commandBuffer = AllocateCommandBuffer();
        }
        {
            frames[i].fence = device->createFenceUnique(
                vk::FenceCreateInfo()
                .setFlags(vk::FenceCreateFlagBits::eSignaled)
            );
        }
        {
            frameSemaphores[i].imageAcquiredSemaphore = device->createSemaphoreUnique({});
            frameSemaphores[i].renderCompleteSemaphore = device->createSemaphoreUnique({});
        }
    }
}

std::vector<vk::UniqueCommandBuffer> Context::AllocateCommandBuffers(uint32_t count)
{
    return device->allocateCommandBuffersUnique(
        vk::CommandBufferAllocateInfo()
        .setCommandPool(*commandPool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(count)
    );
}

vk::UniqueCommandBuffer Context::AllocateCommandBuffer()
{
    return std::move(AllocateCommandBuffers(1).front());
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
    const auto renderArea = vk::Rect2D()
        .setExtent({ static_cast<uint32_t>(Window::GetWidth()), static_cast<uint32_t>(Window::GetHeight()) });

    const auto beginInfo = vk::RenderPassBeginInfo()
        .setRenderPass(*renderPass)
        .setFramebuffer(*frames[frameIndex].framebuffer)
        .setRenderArea(renderArea);

    frames[frameIndex].commandBuffer->beginRenderPass(beginInfo, vk::SubpassContents::eInline);
}

void Context::EndRenderPass()
{
    frames[frameIndex].commandBuffer->endRenderPass();
}

void Context::WaitNextFrame()
{
    const vk::Semaphore imageAcquiredSemaphore = *frameSemaphores[semaphoreIndex].imageAcquiredSemaphore;
    const vk::Semaphore renderCompleteSemaphore = *frameSemaphores[semaphoreIndex].renderCompleteSemaphore;
    try {
        frameIndex = device->acquireNextImageKHR(*swapchain, UINT64_MAX, imageAcquiredSemaphore).value;
    } catch (const std::exception& exception) {
        swapchainRebuild = true;
        return;
    }

    const vk::Fence fence = *frames[frameIndex].fence;
    device->waitForFences(fence, VK_TRUE, UINT64_MAX);
    device->resetFences(fence);
}

vk::CommandBuffer Context::BeginCommandBuffer()
{
    const vk::CommandBufferBeginInfo info{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    frames[frameIndex].commandBuffer->begin(info);
    return *frames[frameIndex].commandBuffer;
}

void Context::Submit(vk::CommandBuffer commandBuffer)
{
    commandBuffer.end();

    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    const auto submitInfo = vk::SubmitInfo()
        .setWaitDstStageMask(waitStage)
        .setCommandBuffers(commandBuffer)
        .setWaitSemaphores(*frameSemaphores[semaphoreIndex].imageAcquiredSemaphore)
        .setSignalSemaphores(*frameSemaphores[semaphoreIndex].renderCompleteSemaphore);

    queue.submit(submitInfo, *frames[frameIndex].fence);
}

void Context::Present()
{
    if (swapchainRebuild) {
        return;
    }

    try {
        queue.presentKHR(
            vk::PresentInfoKHR()
            .setWaitSemaphores(*frameSemaphores[semaphoreIndex].renderCompleteSemaphore)
            .setSwapchains(*swapchain)
            .setImageIndices(frameIndex)
        );
    } catch (const std::exception& exception) {
        std::cerr << "failed to present." << std::endl;
        swapchainRebuild = true;
        return;
    }
    semaphoreIndex = (semaphoreIndex + 1) % swapchainImages.size();
}

void Context::RebuildSwapchain()
{
    //int width, height;
    //glfwGetFramebufferSize(window, &width, &height);
    //if (width > 0 && height > 0) {
    //    ImGui_ImplVulkan_SetMinImageCount(minImageCount);
    //    ImGui_ImplVulkanH_CreateOrResizeWindow(instance, physicalDevice, device, &windowData, Context::queueFamily, nullptr, width, height, minImageCount);
    //    windowData.FrameIndex = 0;
    //    swapchainRebuild = false;
    //}
}
