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

    vk::Instance CreateInstance(const std::vector<const char*>& extensions,
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
        return vk::createInstance(InstanceCI);
    }

    vk::DebugUtilsMessengerEXT CreateDebugMessenger(vk::Instance instance)
    {
        vk::DebugUtilsMessengerCreateInfoEXT debugMessangerCI{};
        debugMessangerCI.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning);
        debugMessangerCI.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
        debugMessangerCI.setPfnUserCallback(&DebugMessage);
        return instance.createDebugUtilsMessengerEXT(debugMessangerCI);
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

    vk::Device CreateDevice(vk::PhysicalDevice physicalDevice, uint32_t queueFamily)
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

        return physicalDevice.createDevice(createInfoChain.get<vk::DeviceCreateInfo>());
    }

    vk::SwapchainKHR CreateSwapchain()
    {
        vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
        swapchainCreateInfo.setSurface(Vulkan::surface);
        swapchainCreateInfo.setMinImageCount(3);
        swapchainCreateInfo.setImageFormat(vk::Format::eB8G8R8A8Unorm);
        swapchainCreateInfo.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear);
        swapchainCreateInfo.setImageExtent({ static_cast<uint32_t>(Window::GetWidth()), static_cast<uint32_t>(Window::GetHeight()) });
        swapchainCreateInfo.setImageArrayLayers(1);
        swapchainCreateInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst);
        swapchainCreateInfo.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity);
        swapchainCreateInfo.setPresentMode(vk::PresentModeKHR::eFifo);
        swapchainCreateInfo.setClipped(true);
        return Vulkan::device.createSwapchainKHR(swapchainCreateInfo);
    }

    std::vector<vk::ImageView> CreateImageViews()
    {
        vk::ImageViewCreateInfo info{};
        info.viewType = vk::ImageViewType::e2D;
        info.format = vk::Format::eB8G8R8A8Unorm;
        info.components.r = vk::ComponentSwizzle::eR;
        info.components.g = vk::ComponentSwizzle::eG;
        info.components.b = vk::ComponentSwizzle::eB;
        info.components.a = vk::ComponentSwizzle::eA;
        info.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

        std::vector<vk::ImageView> views;
        for (uint32_t i = 0; i < Vulkan::swapchainImages.size(); i++) {
            info.image = Vulkan::swapchainImages[i];
            views.push_back(Vulkan::device.createImageView(info));
        }
        return views;
    }

    vk::CommandPool CreateCommandPool(vk::Device device, uint32_t queueFamily)
    {
        vk::CommandPoolCreateInfo commandPoolCI;
        commandPoolCI.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        commandPoolCI.setQueueFamilyIndex(queueFamily);
        return device.createCommandPool(commandPoolCI);
    }

    std::vector<vk::CommandBuffer> CreateCommandBuffers(vk::Device device, vk::CommandPool commandPool, uint32_t count)
    {
        vk::CommandBufferAllocateInfo commandBufferAI;
        commandBufferAI.setCommandPool(commandPool);
        commandBufferAI.setLevel(vk::CommandBufferLevel::ePrimary);
        commandBufferAI.setCommandBufferCount(count);
        return device.allocateCommandBuffers(commandBufferAI);
    }

    vk::CommandBuffer CreateCommandBuffer(vk::Device device, vk::CommandPool commandPool)
    {
        return std::move(CreateCommandBuffers(device, commandPool, 1).front());
    }

    vk::DescriptorPool CraeteDescriptorPool(vk::Device device)
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
        return device.createDescriptorPool(descPoolCI);
    }

    std::vector<const char*> GetExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        return extensions;
    }

    vk::SurfaceKHR CreateSurface()
    {
        VkSurfaceKHR _surface;
        if (glfwCreateWindowSurface(VkInstance(Vulkan::instance), Window::GetWindow(), nullptr, &_surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
        return { _surface };
    }
}

void Vulkan::Init()
{
    spdlog::info("Vulkan::Init()");
    std::vector layers{ "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
    instance = CreateInstance(GetExtensions(), layers);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
    debugMessenger = CreateDebugMessenger(instance);
    physicalDevice = instance.enumeratePhysicalDevices().front();
    surface = CreateSurface();
    queueFamily = FindQueueFamily(physicalDevice, surface);
    device = CreateDevice(physicalDevice, queueFamily);
    queue = device.getQueue(queueFamily, 0);
    commandPool = CreateCommandPool(device, queueFamily);
    swapchain = CreateSwapchain();
    swapchainImages = device.getSwapchainImagesKHR(swapchain);
    swapchainImageViews = CreateImageViews();
    descriptorPool = CraeteDescriptorPool(device);
}

void Vulkan::Shutdown()
{
    spdlog::info("Vulkan::Shutdown()");
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    for (auto&& semaphores : frameSemaphores) {
        Vulkan::device.destroySemaphore(semaphores.imageAcquiredSemaphore);
        Vulkan::device.destroySemaphore(semaphores.renderCompleteSemaphore);
    }
    for (auto&& frame : frames) {
        Vulkan::device.freeCommandBuffers(Vulkan::commandPool, frame.commandBuffer);
        Vulkan::device.destroyFence(frame.fence);
        Vulkan::device.destroyFramebuffer(frame.framebuffer);
    }
    Vulkan::device.destroyPipeline(pipeline);
    Vulkan::device.destroyRenderPass(renderPass);

    device.destroyDescriptorPool(descriptorPool);
    device.destroyCommandPool(commandPool);
    for (auto&& view : swapchainImageViews) {
        device.destroyImageView(view);
    }
    device.destroySwapchainKHR(swapchain);
    device.destroy();
    instance.destroySurfaceKHR(surface);
    instance.destroyDebugUtilsMessengerEXT(debugMessenger);
    instance.destroy();
}

std::vector<vk::UniqueCommandBuffer> Vulkan::AllocateCommandBuffers(uint32_t count)
{
    vk::CommandBufferAllocateInfo commandBufferInfo;
    commandBufferInfo.setCommandPool(Vulkan::commandPool);
    commandBufferInfo.setLevel(vk::CommandBufferLevel::ePrimary);
    commandBufferInfo.setCommandBufferCount(count);
    return Vulkan::device.allocateCommandBuffersUnique(commandBufferInfo);
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
    vk::PhysicalDeviceMemoryProperties memoryProperties = Vulkan::physicalDevice.getMemoryProperties();
    for (uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index) {
        auto propertyFlags = memoryProperties.memoryTypes[index].propertyFlags;
        bool match = (propertyFlags & memoryProp) == memoryProp;
        if (requirements.memoryTypeBits & (1 << index) && match) {
            return index;
        }
    }
    throw std::runtime_error("Failed to find memory type index.");
}

void Vulkan::SetupUI()
{
    spdlog::info("Window::SetupUI()");

    // Create the Render Pass
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
        renderPass = Vulkan::device.createRenderPass(info);
    }

    // Create Framebuffer
    size_t imageCount = Vulkan::swapchainImages.size();
    frames.resize(imageCount);
    frameSemaphores.resize(imageCount);
    {
        vk::ImageView attachment[1];
        vk::FramebufferCreateInfo info{};
        info.renderPass = renderPass;
        info.attachmentCount = 1;
        info.pAttachments = attachment;
        info.width = Window::GetWidth();
        info.height = Window::GetHeight();
        info.layers = 1;

        for (uint32_t i = 0; i < imageCount; i++) {
            attachment[0] = Vulkan::swapchainImageViews[i];
            frames[i].framebuffer = Vulkan::device.createFramebuffer(info);
        }
    }

    // Create Command Buffers
    for (uint32_t i = 0; i < imageCount; i++) {
        {
            vk::CommandBufferAllocateInfo commandBufferInfo;
            commandBufferInfo.setCommandPool(Vulkan::commandPool);
            commandBufferInfo.setLevel(vk::CommandBufferLevel::ePrimary);
            commandBufferInfo.setCommandBufferCount(1);
            frames[i].commandBuffer = Vulkan::device.allocateCommandBuffers(commandBufferInfo)[0];
        }
        {
            vk::FenceCreateInfo info{};
            info.flags = vk::FenceCreateFlagBits::eSignaled;
            frames[i].fence = Vulkan::device.createFence(info);
        }
        {
            vk::SemaphoreCreateInfo info{};
            frameSemaphores[i].imageAcquiredSemaphore = Vulkan::device.createSemaphore({});
            frameSemaphores[i].renderCompleteSemaphore = Vulkan::device.createSemaphore({});
        }
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(Window::GetWindow(), true);
    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = Vulkan::instance;
    initInfo.PhysicalDevice = Vulkan::physicalDevice;
    initInfo.Device = Vulkan::device;
    initInfo.QueueFamily = Vulkan::queueFamily;
    initInfo.Queue = Vulkan::queue;
    initInfo.PipelineCache = nullptr;
    initInfo.DescriptorPool = Vulkan::descriptorPool;
    initInfo.Subpass = 0;
    initInfo.MinImageCount = minImageCount;
    initInfo.ImageCount = imageCount;
    initInfo.MSAASamples = vk::SampleCountFlagBits::e1;
    initInfo.Allocator = nullptr;
    ImGui_ImplVulkan_Init(&initInfo, renderPass);

    io.Fonts->AddFontFromFileTTF("../asset/Roboto-Medium.ttf", 24.0f);
    {
        Vulkan::OneTimeSubmit(
            [&](vk::CommandBuffer commandBuffer)
            {
                ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
            }
        );
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
}

vk::CommandBuffer Vulkan::GetCurrentCommandBuffer()
{
    return frames[frameIndex].commandBuffer;
}

vk::Image Vulkan::GetBackImage()
{
    return swapchainImages[frameIndex];
}

void Vulkan::StartFrame()
{
    //if (swapchainRebuild) {
    //    RebuildSwapchain();
    //}

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Vulkan::WaitNextFrame()
{
    vk::Semaphore imageAcquiredSemaphore = frameSemaphores[semaphoreIndex].imageAcquiredSemaphore;
    vk::Semaphore renderCompleteSemaphore = frameSemaphores[semaphoreIndex].renderCompleteSemaphore;
    try {
        frameIndex = Vulkan::device.acquireNextImageKHR(Vulkan::swapchain, UINT64_MAX, imageAcquiredSemaphore).value;
    } catch (std::exception exception) {
        swapchainRebuild = true;
        return;
    }

    vk::Fence fence = frames[frameIndex].fence;
    Vulkan::device.waitForFences(fence, VK_TRUE, UINT64_MAX);
    Vulkan::device.resetFences(fence);
}

void Vulkan::BeginCommandBuffer()
{
    vk::CommandBufferBeginInfo info{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    GetCurrentCommandBuffer().begin(info);
}

void Vulkan::Submit()
{
    vk::CommandBuffer commandBuffer = frames[frameIndex].commandBuffer;
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo info{};
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &frameSemaphores[semaphoreIndex].imageAcquiredSemaphore;
    info.pWaitDstStageMask = &waitStage;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &commandBuffer;
    info.signalSemaphoreCount = 1;
    info.pSignalSemaphores = &frameSemaphores[semaphoreIndex].renderCompleteSemaphore;

    commandBuffer.end();
    vk::Fence fence = frames[frameIndex].fence;
    Vulkan::queue.submit(info, fence);
}

void Vulkan::RenderUI()
{
    ImDrawData* drawData = ImGui::GetDrawData();

    vk::RenderPassBeginInfo info{};
    info.renderPass = renderPass;
    info.framebuffer = frames[frameIndex].framebuffer;
    info.renderArea.extent.width = Window::GetWidth();
    info.renderArea.extent.height = Window::GetHeight();
    info.clearValueCount = 1;
    info.pClearValues = &clearValue;
    frames[frameIndex].commandBuffer.beginRenderPass(info, vk::SubpassContents::eInline);

    ImGui_ImplVulkan_RenderDrawData(drawData, frames[frameIndex].commandBuffer);

    frames[frameIndex].commandBuffer.endRenderPass();
}

void Vulkan::Present()
{
    if (swapchainRebuild) {
        return;
    }
    vk::PresentInfoKHR info{};
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &frameSemaphores[semaphoreIndex].renderCompleteSemaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &swapchain;
    info.pImageIndices = &frameIndex;
    try {
        Vulkan::queue.presentKHR(info);
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
    //    ImGui_ImplVulkanH_CreateOrResizeWindow(Vulkan::instance, Vulkan::physicalDevice, Vulkan::device, &windowData, Vulkan::queueFamily, nullptr, width, height, minImageCount);
    //    windowData.FrameIndex = 0;
    //    swapchainRebuild = false;
    //}
}
