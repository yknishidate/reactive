#include "Helper.hpp"
#include "Window.hpp"

VKAPI_ATTR VkBool32 VKAPI_CALL Helper::DebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData, void* pUserData)
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

vk::UniqueInstance Helper::CreateInstance(const std::vector<const char*>& extensions, const std::vector<const char*>& layers)
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

vk::UniqueDebugUtilsMessengerEXT Helper::CreateDebugMessenger(vk::Instance instance)
{
    return instance.createDebugUtilsMessengerEXTUnique(
        vk::DebugUtilsMessengerCreateInfoEXT()
        .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
        .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
        .setPfnUserCallback(&DebugMessage)
    );
}

uint32_t Helper::FindQueueFamily(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
{
    const std::vector properties = physicalDevice.getQueueFamilyProperties();
    for (uint32_t index = 0; index < properties.size(); index++) {
        const auto supportGraphics = properties[index].queueFlags & vk::QueueFlagBits::eGraphics;
        const auto supportCompute = properties[index].queueFlags & vk::QueueFlagBits::eCompute;
        const auto supportPresent = physicalDevice.getSurfaceSupportKHR(index, surface);
        if (supportGraphics && supportCompute && supportPresent) {
            return index;
        }
    }
    throw std::runtime_error("Failed to find queue family.");
}

vk::UniqueDevice Helper::CreateDevice(vk::PhysicalDevice physicalDevice, uint32_t queueFamily)
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
        createInfoChain{ deviceInfo,{ true },{ true },{ true }, descFeatures };

    return physicalDevice.createDeviceUnique(createInfoChain.get<vk::DeviceCreateInfo>());
}

vk::UniqueCommandPool Helper::CreateCommandPool(vk::Device device, uint32_t queueFamily)
{
    return device.createCommandPoolUnique(
        vk::CommandPoolCreateInfo()
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
        .setQueueFamilyIndex(queueFamily)
    );
}

vk::UniqueDescriptorPool Helper::CraeteDescriptorPool(vk::Device device)
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

vk::UniqueSwapchainKHR Helper::CreateSwapchain(vk::Device device, vk::SurfaceKHR surface, uint32_t minImageCount, uint32_t queueFamily)
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

std::vector<vk::UniqueImageView> Helper::CreateImageViews(vk::Device device, const std::vector<vk::Image>& images)
{
    std::vector<vk::UniqueImageView> views;
    for (auto&& image : images) {
        views.push_back(device.createImageViewUnique(
            vk::ImageViewCreateInfo()
            .setImage(image)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(vk::Format::eB8G8R8A8Unorm)
            .setComponents({ vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA })
            .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 })
        ));
    }
    return views;
}

vk::UniqueRenderPass Helper::CreateRenderPass(vk::Device device)
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
