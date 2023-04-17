#include "App.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

App::App(int width, int height, const std::string& title, bool enableValidation)
    : m_width{width}, m_height{height}, m_title{title}, m_enableValidation{enableValidation} {
    spdlog::set_pattern("[%^%l%$] %v");

    try {
        initGLFW();
        initVulkan();
        initImGui();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}

void App::run() {
    try {
        onStart();
        while (!glfwWindowShouldClose(m_window)) {
            glfwPollEvents();

            // Update mouse position
            double xPos{};
            double yPos{};
            glfwGetCursorPos(m_window, &xPos, &yPos);
            m_lastMousePos = m_currMousePos;
            m_currMousePos = {xPos, yPos};

            onUpdate();

            // Start ImGui
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            onRender();
        }
        device->waitIdle();

        // Shutdown GLFW
        glfwDestroyWindow(m_window);
        glfwTerminate();

        // Shutdown ImGui
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}

std::vector<vk::UniqueCommandBuffer> App::allocateCommandBuffers(uint32_t count) const {
    return device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo()
                                                    .setCommandPool(*commandPool)
                                                    .setLevel(vk::CommandBufferLevel::ePrimary)
                                                    .setCommandBufferCount(count));
}

void App::oneTimeSubmit(const std::function<void(vk::CommandBuffer)>& command) const {
    vk::UniqueCommandBuffer commandBuffer = std::move(allocateCommandBuffers(1).front());
    commandBuffer->begin(
        vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    command(*commandBuffer);
    commandBuffer->end();
    queue.submit(vk::SubmitInfo().setCommandBuffers(*commandBuffer));
    queue.waitIdle();
}

vk::UniqueDescriptorSet App::allocateDescriptorSet(vk::DescriptorSetLayout descSetLayout) const {
    vk::DescriptorSetAllocateInfo descSetInfo;
    descSetInfo.setDescriptorPool(*descriptorPool);
    descSetInfo.setSetLayouts(descSetLayout);
    return std::move(device->allocateDescriptorSetsUnique(descSetInfo).front());
}

uint32_t App::findMemoryTypeIndex(vk::MemoryRequirements requirements,
                                  vk::MemoryPropertyFlags memoryProp) const {
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i != memProperties.memoryTypeCount; ++i) {
        if ((requirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & memoryProp) == memoryProp) {
            return i;
        }
    }
    throw std::runtime_error("Failed to find memory type index.");
}

bool App::mousePressed() const {
    bool pressed = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (ImGui::GetCurrentContext()) {
        return pressed && !ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
    }
    return pressed;
}

void App::initGLFW() {
    // Init GLFW
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Create window
    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);

    // Set window icon
    GLFWimage icon;
    std::string iconPath = ASSET_DIR + "Vulkan.png";
    icon.pixels = stbi_load(iconPath.c_str(), &icon.width, &icon.height, nullptr, 4);
    if (icon.pixels != nullptr) {
        glfwSetWindowIcon(m_window, 1, &icon);
    }
    stbi_image_free(icon.pixels);
}

void App::initVulkan() {
    static const vk::DynamicLoader dl;
    auto vkGetInstanceProcAddr =
        dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    // Create instance
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector instanceExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    std::vector layers{"VK_LAYER_LUNARG_monitor"};
    if (m_enableValidation) {
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    vk::ApplicationInfo appInfo;
    appInfo.setApiVersion(VK_API_VERSION_1_3);
    instance = vk::createInstanceUnique(vk::InstanceCreateInfo()
                                            .setPApplicationInfo(&appInfo)
                                            .setPEnabledExtensionNames(instanceExtensions)
                                            .setPEnabledLayerNames(layers));
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

    // Create debug messenger
    if (m_enableValidation) {
        debugMessenger = instance->createDebugUtilsMessengerEXTUnique(
            vk::DebugUtilsMessengerCreateInfoEXT()
                .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
                .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
                .setPfnUserCallback(&debugCallback));
    }

    // Create surface
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(VkInstance{*instance}, m_window, nullptr, &_surface) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    surface = vk::UniqueSurfaceKHR{_surface, {*instance}};

    // Pick GPU
    physicalDevice = instance->enumeratePhysicalDevices().front();

    // Find queue family
    std::vector properties = physicalDevice.getQueueFamilyProperties();
    for (uint32_t index = 0; index < properties.size(); index++) {
        auto supportGraphics = properties[index].queueFlags & vk::QueueFlagBits::eGraphics;
        auto supportCompute = properties[index].queueFlags & vk::QueueFlagBits::eCompute;
        if (surface) {
            auto supportPresent = physicalDevice.getSurfaceSupportKHR(index, *surface);
            if (supportGraphics && supportCompute && supportPresent) {
                queueFamily = index;
            }
        } else {
            if (supportGraphics && supportCompute) {
                queueFamily = index;
            }
        }
    }
    if (queueFamily == ~0u) {
        throw std::runtime_error("Failed to find queue family.");
    }

    // Create device
    float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo queueInfo;
    queueInfo.setQueueFamilyIndex(queueFamily);
    queueInfo.setQueueCount(1);
    queueInfo.setPQueuePriorities(&queuePriority);

    std::vector deviceExtensions{
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
        VK_EXT_MESH_SHADER_EXTENSION_NAME,
        VK_KHR_SPIRV_1_4_EXTENSION_NAME,
        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
        VK_KHR_RAY_QUERY_EXTENSION_NAME,
    };

    vk::PhysicalDeviceFeatures deviceFeatures;
    deviceFeatures.setShaderInt64(true);
    deviceFeatures.setFragmentStoresAndAtomics(true);
    deviceFeatures.setVertexPipelineStoresAndAtomics(true);
    deviceFeatures.setGeometryShader(true);
    deviceFeatures.setFillModeNonSolid(true);
    deviceFeatures.setWideLines(true);

    vk::PhysicalDeviceDescriptorIndexingFeatures descFeatures;
    descFeatures.setRuntimeDescriptorArray(true);

    vk::PhysicalDevice8BitStorageFeatures storage8BitFeatures;
    storage8BitFeatures.setStorageBuffer8BitAccess(true);

    vk::PhysicalDeviceShaderFloat16Int8Features shaderFloat16Int8Features;
    shaderFloat16Int8Features.setShaderInt8(true);

    vk::DeviceCreateInfo deviceInfo;
    deviceInfo.setQueueCreateInfos(queueInfo);
    deviceInfo.setPEnabledExtensionNames(deviceExtensions);
    deviceInfo.setPEnabledFeatures(&deviceFeatures);

    vk::StructureChain createInfoChain{deviceInfo,
                                       vk::PhysicalDeviceBufferDeviceAddressFeatures{true},
                                       vk::PhysicalDeviceRayTracingPipelineFeaturesKHR{true},
                                       vk::PhysicalDeviceAccelerationStructureFeaturesKHR{true},
                                       vk::PhysicalDeviceMeshShaderFeaturesEXT{true, true},
                                       vk::PhysicalDeviceRayQueryFeaturesKHR{true},
                                       descFeatures,
                                       shaderFloat16Int8Features,
                                       storage8BitFeatures};

    device = physicalDevice.createDeviceUnique(createInfoChain.get<vk::DeviceCreateInfo>());

    queue = device->getQueue(queueFamily, 0);

    // Create command pool
    commandPool = device->createCommandPoolUnique(
        vk::CommandPoolCreateInfo()
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
            .setQueueFamilyIndex(queueFamily));

    // Create descriptor pool
    std::vector<vk::DescriptorPoolSize> poolSizes{
        {vk::DescriptorType::eSampler, 100},
        {vk::DescriptorType::eCombinedImageSampler, 100},
        {vk::DescriptorType::eSampledImage, 100},
        {vk::DescriptorType::eStorageImage, 100},
        {vk::DescriptorType::eUniformBuffer, 100},
        {vk::DescriptorType::eStorageBuffer, 100},
        {vk::DescriptorType::eInputAttachment, 100},
        {vk::DescriptorType::eAccelerationStructureKHR, 100}};

    descriptorPool = device->createDescriptorPoolUnique(
        vk::DescriptorPoolCreateInfo().setPoolSizes(poolSizes).setMaxSets(100).setFlags(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet));

    // Create swapchain
    swapchain = device->createSwapchainKHRUnique(
        vk::SwapchainCreateInfoKHR()
            .setSurface(*surface)
            .setMinImageCount(minImageCount)
            .setImageFormat(vk::Format::eB8G8R8A8Unorm)
            .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
            .setImageExtent({static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height)})
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment |
                           vk::ImageUsageFlagBits::eTransferDst)
            .setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
            .setPresentMode(vk::PresentModeKHR::eFifo)
            .setClipped(true)
            .setQueueFamilyIndices(queueFamily));

    // Get images
    swapchainImages = device->getSwapchainImagesKHR(*swapchain);

    // Create image views
    for (auto& image : swapchainImages) {
        swapchainImageViews.push_back(device->createImageViewUnique(
            vk::ImageViewCreateInfo()
                .setImage(image)
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(vk::Format::eB8G8R8A8Unorm)
                .setComponents({vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
                                vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA})
                .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})));
    }

    // Create depth image
    depthImage = device->createImageUnique(
        vk::ImageCreateInfo()
            .setImageType(vk::ImageType::e2D)
            .setFormat(vk::Format::eD32Sfloat)
            .setExtent({static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height), 1})
            .setMipLevels(1)
            .setArrayLayers(1)
            .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment));

    vk::MemoryRequirements requirements = device->getImageMemoryRequirements(*depthImage);
    uint32_t memoryTypeIndex =
        findMemoryTypeIndex(requirements, vk::MemoryPropertyFlagBits::eDeviceLocal);
    depthImageMemory = device->allocateMemoryUnique(vk::MemoryAllocateInfo()
                                                        .setAllocationSize(requirements.size)
                                                        .setMemoryTypeIndex(memoryTypeIndex));

    device->bindImageMemory(*depthImage, *depthImageMemory, 0);

    depthImageView = device->createImageViewUnique(
        vk::ImageViewCreateInfo()
            .setImage(*depthImage)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(vk::Format::eD32Sfloat)
            .setSubresourceRange({vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}));

    // Create render pass
    vk::AttachmentDescription colorAttachment;
    colorAttachment.setFormat(vk::Format::eB8G8R8A8Unorm);
    colorAttachment.setSamples(vk::SampleCountFlagBits::e1);
    colorAttachment.setLoadOp(vk::AttachmentLoadOp::eDontCare);
    colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
    colorAttachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    colorAttachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    colorAttachment.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentDescription depthAttachment;
    depthAttachment.setFormat(vk::Format::eD32Sfloat);
    depthAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
    depthAttachment.setStoreOp(vk::AttachmentStoreOp::eDontCare);
    depthAttachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    depthAttachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    depthAttachment.setInitialLayout(vk::ImageLayout::eUndefined);
    depthAttachment.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::AttachmentReference colorAttachmentRef;
    colorAttachmentRef.setAttachment(0);
    colorAttachmentRef.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentReference depthAttachmentRef;
    depthAttachmentRef.setAttachment(1);
    depthAttachmentRef.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpass.setColorAttachments(colorAttachmentRef);
    subpass.setPDepthStencilAttachment(&depthAttachmentRef);

    vk::SubpassDependency dependency;
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL);
    dependency.setDstSubpass(0);
    dependency.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput |
                               vk::PipelineStageFlagBits::eEarlyFragmentTests);
    dependency.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput |
                               vk::PipelineStageFlagBits::eEarlyFragmentTests);
    dependency.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite |
                                vk::AccessFlagBits::eDepthStencilAttachmentWrite);

    std::array attachments{colorAttachment, depthAttachment};

    renderPass = device->createRenderPassUnique(vk::RenderPassCreateInfo()
                                                    .setAttachments(attachments)
                                                    .setSubpasses(subpass)
                                                    .setDependencies(dependency));

    size_t imageCount = swapchainImages.size();
    commandBuffers = allocateCommandBuffers(imageCount);
    framebuffers.resize(imageCount);
    fences.resize(imageCount);
    imageAcquiredSemaphore = device->createSemaphoreUnique({});
    renderCompleteSemaphore = device->createSemaphoreUnique({});
    for (uint32_t i = 0; i < imageCount; i++) {
        std::array attachments{*swapchainImageViews[i], *depthImageView};
        framebuffers[i] = device->createFramebufferUnique(vk::FramebufferCreateInfo()
                                                              .setRenderPass(*renderPass)
                                                              .setAttachments(attachments)
                                                              .setWidth(m_width)
                                                              .setHeight(m_height)
                                                              .setLayers(1));
        fences[i] = device->createFenceUnique(
            vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled));
    }
}

void App::initImGui() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();

    // Color scheme
    ImVec4 red80 = ImVec4(0.8f, 0.0f, 0.0f, 1.0f);
    ImVec4 red60 = ImVec4(0.6f, 0.0f, 0.0f, 1.0f);
    ImVec4 red40 = ImVec4(0.4f, 0.0f, 0.0f, 1.0f);
    ImVec4 black100 = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    ImVec4 black90 = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    ImVec4 black80 = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    ImVec4 black60 = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);

    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = black90;
    style.Colors[ImGuiCol_TitleBg] = red80;
    style.Colors[ImGuiCol_TitleBgActive] = red80;
    style.Colors[ImGuiCol_MenuBarBg] = red40;
    style.Colors[ImGuiCol_Header] = red40;
    style.Colors[ImGuiCol_HeaderActive] = red40;
    style.Colors[ImGuiCol_HeaderHovered] = red40;
    style.Colors[ImGuiCol_FrameBg] = black100;
    style.Colors[ImGuiCol_FrameBgHovered] = black80;
    style.Colors[ImGuiCol_FrameBgActive] = black60;
    style.Colors[ImGuiCol_CheckMark] = red80;
    style.Colors[ImGuiCol_SliderGrab] = red40;
    style.Colors[ImGuiCol_SliderGrabActive] = red80;
    style.Colors[ImGuiCol_Button] = red40;
    style.Colors[ImGuiCol_ButtonHovered] = red60;
    style.Colors[ImGuiCol_ButtonActive] = red80;
    style.Colors[ImGuiCol_ResizeGrip] = red40;
    style.Colors[ImGuiCol_ResizeGripHovered] = red60;
    style.Colors[ImGuiCol_ResizeGripActive] = red80;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(m_window, true);
    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = *instance;
    initInfo.PhysicalDevice = physicalDevice;
    initInfo.Device = *device;
    initInfo.QueueFamily = queueFamily;
    initInfo.Queue = queue;
    initInfo.PipelineCache = nullptr;
    initInfo.DescriptorPool = *descriptorPool;
    initInfo.Subpass = 0;
    initInfo.MinImageCount = minImageCount;
    initInfo.ImageCount = swapchainImages.size();
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.Allocator = nullptr;
    ImGui_ImplVulkan_Init(&initInfo, *renderPass);

    // Setup font
    std::string fontFile = ASSET_DIR + "Roboto-Medium.ttf";
    io.Fonts->AddFontFromFileTTF(fontFile.c_str(), 24.0f);
    {
        oneTimeSubmit([&](vk::CommandBuffer commandBuffer) {
            ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
        });
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
}
