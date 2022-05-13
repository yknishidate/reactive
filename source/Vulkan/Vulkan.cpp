#include "Vulkan.hpp"
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
        std::string message{ pCallbackData->pMessage };
        std::regex regex{ "The Vulkan spec states: " };
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
        static vk::DynamicLoader dl;
        auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        vk::ApplicationInfo appInfo;
        appInfo.apiVersion = VK_API_VERSION_1_2;

        vk::InstanceCreateInfo InstanceCI{};
        InstanceCI.setPApplicationInfo(&appInfo);
        InstanceCI.setPEnabledExtensionNames(extensions);
        InstanceCI.setPEnabledLayerNames(layers);
        return vk::createInstanceUnique(InstanceCI);
    }

    vk::UniqueDebugUtilsMessengerEXT CreateDebugMessenger(vk::Instance instance)
    {
        vk::DebugUtilsMessengerCreateInfoEXT debugMessangerCI{};
        debugMessangerCI.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning);
        debugMessangerCI.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
        debugMessangerCI.setPfnUserCallback(&DebugMessage);
        return instance.createDebugUtilsMessengerEXTUnique(debugMessangerCI);
    }

    uint32_t FindQueueFamily(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
    {
        std::vector queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
        for (uint32_t index = 0; index < queueFamilyProperties.size(); index++) {
            auto supportGraphics = queueFamilyProperties[index].queueFlags & vk::QueueFlagBits::eGraphics;
            auto supportCompute = queueFamilyProperties[index].queueFlags & vk::QueueFlagBits::eCompute;
            auto supportPresent = physicalDevice.getSurfaceSupportKHR(index, surface);
            if (supportGraphics && supportCompute && supportPresent) {
                return index;
            }
        }
        throw std::runtime_error("Failed to find queue family.");
    }

    vk::UniqueDevice CreateDevice(vk::PhysicalDevice physicalDevice, uint32_t queueFamily)
    {
        float queuePriority = 0.0f;
        vk::DeviceQueueCreateInfo queueCI;
        queueCI.setQueueFamilyIndex(queueFamily);
        queueCI.setQueueCount(1);
        queueCI.setPQueuePriorities(&queuePriority);

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

        vk::PhysicalDeviceFeatures deviceFeatures;
        deviceFeatures.shaderInt64 = true;
        deviceFeatures.fragmentStoresAndAtomics = true;
        deviceFeatures.vertexPipelineStoresAndAtomics = true;

        vk::PhysicalDeviceDescriptorIndexingFeatures descFeatures;
        descFeatures.runtimeDescriptorArray = true;

        vk::DeviceCreateInfo createInfo{ {}, queueCI, {}, deviceExtensions, &deviceFeatures };
        vk::StructureChain<vk::DeviceCreateInfo,
            vk::PhysicalDeviceBufferDeviceAddressFeatures,
            vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
            vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
            vk::PhysicalDeviceDescriptorIndexingFeatures>
            createInfoChain{ createInfo, {true}, {true}, {true}, descFeatures };

        return physicalDevice.createDeviceUnique(createInfoChain.get<vk::DeviceCreateInfo>());
    }

    vk::UniqueSwapchainKHR CreateSwapchain(vk::Device device, vk::SurfaceKHR surface, uint32_t minImageCount, uint32_t queueFamily)
    {
        vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
        swapchainCreateInfo.setSurface(surface);
        swapchainCreateInfo.setMinImageCount(minImageCount);
        swapchainCreateInfo.setImageFormat(vk::Format::eB8G8R8A8Unorm);
        swapchainCreateInfo.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear);
        swapchainCreateInfo.setImageExtent({ static_cast<uint32_t>(Window::GetWidth()), static_cast<uint32_t>(Window::GetHeight()) });
        swapchainCreateInfo.setImageArrayLayers(1);
        swapchainCreateInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst);
        swapchainCreateInfo.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity);
        swapchainCreateInfo.setPresentMode(vk::PresentModeKHR::eFifo);
        swapchainCreateInfo.setClipped(true);
        swapchainCreateInfo.setQueueFamilyIndices(queueFamily);
        return device.createSwapchainKHRUnique(swapchainCreateInfo);
    }

    std::vector<vk::UniqueImageView> CreateImageViews(vk::Device device, const std::vector<vk::Image>& swapchainImages)
    {
        vk::ImageViewCreateInfo info{};
        info.viewType = vk::ImageViewType::e2D;
        info.format = vk::Format::eB8G8R8A8Unorm;
        info.components.r = vk::ComponentSwizzle::eR;
        info.components.g = vk::ComponentSwizzle::eG;
        info.components.b = vk::ComponentSwizzle::eB;
        info.components.a = vk::ComponentSwizzle::eA;
        info.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

        std::vector<vk::UniqueImageView> views;
        for (uint32_t i = 0; i < swapchainImages.size(); i++) {
            info.image = swapchainImages[i];
            views.push_back(device.createImageViewUnique(info));
        }
        return views;
    }

    vk::UniqueCommandPool CreateCommandPool(vk::Device device, uint32_t queueFamily)
    {
        vk::CommandPoolCreateInfo commandPoolCI;
        commandPoolCI.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        commandPoolCI.setQueueFamilyIndex(queueFamily);
        return device.createCommandPoolUnique(commandPoolCI);
    }

    std::vector<vk::UniqueCommandBuffer> CreateCommandBuffers(vk::Device device, vk::CommandPool commandPool, uint32_t count)
    {
        vk::CommandBufferAllocateInfo commandBufferAI;
        commandBufferAI.setCommandPool(commandPool);
        commandBufferAI.setLevel(vk::CommandBufferLevel::ePrimary);
        commandBufferAI.setCommandBufferCount(count);
        return device.allocateCommandBuffersUnique(commandBufferAI);
    }

    vk::UniqueCommandBuffer CreateCommandBuffer(vk::Device device, vk::CommandPool commandPool)
    {
        return std::move(CreateCommandBuffers(device, commandPool, 1).front());
    }

    vk::UniqueDescriptorPool CraeteDescriptorPool(vk::Device device)
    {
        std::vector<vk::DescriptorPoolSize> poolSizes{
            { vk::DescriptorType::eSampler, 100 },
            { vk::DescriptorType::eCombinedImageSampler, 100 },
            { vk::DescriptorType::eSampledImage, 100 },
            { vk::DescriptorType::eStorageImage, 100 },
            { vk::DescriptorType::eUniformBuffer, 100 },
            { vk::DescriptorType::eStorageBuffer, 100 },
            { vk::DescriptorType::eInputAttachment, 100 }
        };

        vk::DescriptorPoolCreateInfo descPoolCI;
        descPoolCI.setPoolSizes(poolSizes);
        descPoolCI.setMaxSets(100);
        descPoolCI.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
        return device.createDescriptorPoolUnique(descPoolCI);
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
        vk::AttachmentDescription attachment{};
        attachment.format = vk::Format::eB8G8R8A8Unorm;
        attachment.samples = vk::SampleCountFlagBits::e1;
        attachment.loadOp = vk::AttachmentLoadOp::eDontCare;
        attachment.storeOp = vk::AttachmentStoreOp::eStore;
        attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachment.initialLayout = vk::ImageLayout::eUndefined;
        attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

        vk::AttachmentReference color_attachment{};
        color_attachment.attachment = 0;
        color_attachment.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::SubpassDescription subpass{};
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment;

        vk::SubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.srcAccessMask = {};
        dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

        vk::RenderPassCreateInfo info{};
        info.attachmentCount = 1;
        info.pAttachments = &attachment;
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        info.dependencyCount = 1;
        info.pDependencies = &dependency;
        return device.createRenderPassUnique(info);
    }
}

void Vulkan::Init()
{
    spdlog::info("Vulkan::Init()");
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
    size_t imageCount = Vulkan::swapchainImages.size();
    frames.resize(imageCount);
    frameSemaphores.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++) {
        {
            vk::ImageView attachment[1];
            vk::FramebufferCreateInfo info{};
            info.renderPass = *renderPass;
            info.attachmentCount = 1;
            info.pAttachments = attachment;
            info.width = Window::GetWidth();
            info.height = Window::GetHeight();
            info.layers = 1;
            attachment[0] = *swapchainImageViews[i];
            frames[i].framebuffer = device->createFramebufferUnique(info);
        }
        {
            vk::CommandBufferAllocateInfo commandBufferInfo;
            commandBufferInfo.setCommandPool(*commandPool);
            commandBufferInfo.setLevel(vk::CommandBufferLevel::ePrimary);
            commandBufferInfo.setCommandBufferCount(1);
            frames[i].commandBuffer = AllocateCommandBuffer();
        }
        {
            vk::FenceCreateInfo info{};
            info.flags = vk::FenceCreateFlagBits::eSignaled;
            frames[i].fence = device->createFenceUnique(info);
        }
        {
            vk::SemaphoreCreateInfo info{};
            frameSemaphores[i].imageAcquiredSemaphore = device->createSemaphoreUnique({});
            frameSemaphores[i].renderCompleteSemaphore = device->createSemaphoreUnique({});
        }
    }
}

std::vector<vk::UniqueCommandBuffer> Vulkan::AllocateCommandBuffers(uint32_t count)
{
    vk::CommandBufferAllocateInfo commandBufferInfo;
    commandBufferInfo.setCommandPool(*commandPool);
    commandBufferInfo.setLevel(vk::CommandBufferLevel::ePrimary);
    commandBufferInfo.setCommandBufferCount(count);
    return device->allocateCommandBuffersUnique(commandBufferInfo);
}

vk::UniqueCommandBuffer Vulkan::AllocateCommandBuffer()
{
    return std::move(AllocateCommandBuffers(1).front());
}

void Vulkan::SubmitAndWait(vk::CommandBuffer commandBuffer)
{
    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(commandBuffer);
    queue.submit(submitInfo);
    queue.waitIdle();
}

void Vulkan::OneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command)
{
    vk::UniqueCommandBuffer commandBuffer = AllocateCommandBuffer();

    vk::CommandBufferBeginInfo beginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    commandBuffer->begin(beginInfo);
    command(*commandBuffer);
    commandBuffer->end();
    SubmitAndWait(*commandBuffer);
}

uint32_t Vulkan::FindMemoryTypeIndex(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags memoryProp)
{
    vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();
    for (uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index) {
        auto propertyFlags = memoryProperties.memoryTypes[index].propertyFlags;
        bool match = (propertyFlags & memoryProp) == memoryProp;
        if (requirements.memoryTypeBits & (1 << index) && match) {
            return index;
        }
    }
    throw std::runtime_error("Failed to find memory type index.");
}

void Vulkan::BeginRenderPass()
{
    vk::ClearValue clearValue{};
    vk::RenderPassBeginInfo info{};
    info.renderPass = *renderPass;
    info.framebuffer = *frames[frameIndex].framebuffer;
    info.renderArea.extent.width = Window::GetWidth();
    info.renderArea.extent.height = Window::GetHeight();
    info.clearValueCount = 1;
    info.pClearValues = &clearValue;
    frames[frameIndex].commandBuffer->beginRenderPass(info, vk::SubpassContents::eInline);
}

void Vulkan::EndRenderPass()
{
    frames[frameIndex].commandBuffer->endRenderPass();
}

void Vulkan::WaitNextFrame()
{
    vk::Semaphore imageAcquiredSemaphore = *frameSemaphores[semaphoreIndex].imageAcquiredSemaphore;
    vk::Semaphore renderCompleteSemaphore = *frameSemaphores[semaphoreIndex].renderCompleteSemaphore;
    try {
        frameIndex = device->acquireNextImageKHR(*swapchain, UINT64_MAX, imageAcquiredSemaphore).value;
    } catch (std::exception exception) {
        swapchainRebuild = true;
        return;
    }

    vk::Fence fence = *frames[frameIndex].fence;
    device->waitForFences(fence, VK_TRUE, UINT64_MAX);
    device->resetFences(fence);
}

vk::CommandBuffer Vulkan::BeginCommandBuffer()
{
    vk::CommandBufferBeginInfo info{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    frames[frameIndex].commandBuffer->begin(info);
    return *frames[frameIndex].commandBuffer;
}

void Vulkan::Submit(vk::CommandBuffer commandBuffer)
{
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo info{};
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &*frameSemaphores[semaphoreIndex].imageAcquiredSemaphore;
    info.pWaitDstStageMask = &waitStage;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &commandBuffer;
    info.signalSemaphoreCount = 1;
    info.pSignalSemaphores = &*frameSemaphores[semaphoreIndex].renderCompleteSemaphore;

    commandBuffer.end();
    queue.submit(info, *frames[frameIndex].fence);
}

void Vulkan::Present()
{
    if (swapchainRebuild) {
        return;
    }
    vk::PresentInfoKHR info{};
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &*frameSemaphores[semaphoreIndex].renderCompleteSemaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &*swapchain;
    info.pImageIndices = &frameIndex;
    try {
        queue.presentKHR(info);
    } catch (std::exception exception) {
        std::cerr << "failed to present." << std::endl;
        swapchainRebuild = true;
        return;
    }
    semaphoreIndex = (semaphoreIndex + 1) % swapchainImages.size();
}

void Vulkan::RebuildSwapchain()
{
    //int width, height;
    //glfwGetFramebufferSize(window, &width, &height);
    //if (width > 0 && height > 0) {
    //    ImGui_ImplVulkan_SetMinImageCount(minImageCount);
    //    ImGui_ImplVulkanH_CreateOrResizeWindow(instance, physicalDevice, device, &windowData, Vulkan::queueFamily, nullptr, width, height, minImageCount);
    //    windowData.FrameIndex = 0;
    //    swapchainRebuild = false;
    //}
}
