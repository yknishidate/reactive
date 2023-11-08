#include "App.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "Graphics/Fence.hpp"

namespace rv {
App::App(AppCreateInfo createInfo) : width{createInfo.width}, height{createInfo.height} {
    spdlog::set_pattern("[%^%l%$] %v");

    presentMode = createInfo.vsync ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eMailbox;

    initGLFW(createInfo.windowResizable, createInfo.title);
    initVulkan(createInfo.layers, createInfo.extensions);
    initImGui();
}

void App::run() {
    onStart();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (width == 0 && height == 0) {
            continue;
        }

        onUpdate();

        // Start ImGui
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Wait fence
        fences[frameIndex]->wait();

        // Acquire next image
        auto acquireResult = context.getDevice().acquireNextImageKHR(
            *swapchain, UINT64_MAX, *imageAcquiredSemaphores[semaphoreIndex]);
        if (acquireResult.result == vk::Result::eErrorOutOfDateKHR) {
            continue;
        }
        frameIndex = acquireResult.value;

        // Reset fence
        fences[frameIndex]->reset();

        // Begin command buffer
        // NOTE: Since the command pool is created with the Reset flag,
        //       the command buffer is implicitly reset at begin.
        commandBuffers[frameIndex]->begin();

        // Render
        onRender(commandBuffers[frameIndex]);

        // Draw GUI
        {
            // Begin render pass
            commandBuffers[frameIndex]->beginRendering(getCurrentColorImage(), {}, {0, 0},
                                                       {width, height});

            // Render
            // TODO: create ImGui wrapper
            ImGui::Render();
            ImDrawData* drawData = ImGui::GetDrawData();
            ImGui_ImplVulkan_RenderDrawData(drawData, *commandBuffers[frameIndex]->commandBuffer);

            // End render pass
            commandBuffers[frameIndex]->endRendering();
        }

        commandBuffers[frameIndex]->transitionLayout(getCurrentColorImage(),
                                                     vk::ImageLayout::ePresentSrcKHR);

        // End command buffer
        commandBuffers[frameIndex]->end();

        // Submit
        vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        context.submit(commandBuffers[frameIndex], waitStage,
                       *imageAcquiredSemaphores[semaphoreIndex],
                       *renderCompleteSemaphores[semaphoreIndex], fences[frameIndex]);

        // Present image
        vk::PresentInfoKHR presentInfo;
        presentInfo.setWaitSemaphores(*renderCompleteSemaphores[semaphoreIndex]);
        presentInfo.setSwapchains(*swapchain);
        presentInfo.setImageIndices(frameIndex);
        if (context.getQueue().presentKHR(presentInfo) != vk::Result::eSuccess) {
            swapchainRebuild = true;
            return;
        }
        semaphoreIndex = (semaphoreIndex + 1) % swapchainImages.size();
    }
    context.getDevice().waitIdle();

    // Shutdown GLFW
    glfwDestroyWindow(window);
    glfwTerminate();

    // Shutdown ImGui
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

auto App::getCursorPos() const -> glm::vec2 {
    double xPos{};
    double yPos{};
    glfwGetCursorPos(window, &xPos, &yPos);
    return {xPos, yPos};
}

auto App::getMouseWheel() const -> glm::vec2 {
    return mouseWheel;
}

auto App::getCurrentColorImage() const -> ImageHandle {
    return std::make_shared<Image>(swapchainImages[frameIndex], *swapchainImageViews[frameIndex],
                                   vk::Extent3D{width, height, 1}, vk::ImageAspectFlagBits::eColor);
}

auto App::getDefaultDepthImage() const -> ImageHandle {
    return depthImage;
}

auto App::isKeyDown(int key) const -> bool {
    ImGuiIO& io = ImGui::GetIO();
    if (key < GLFW_KEY_SPACE || key > GLFW_KEY_LAST || io.WantCaptureKeyboard) {
        return false;
    }
    return glfwGetKey(window, key) == GLFW_PRESS;
}

auto App::isMouseButtonDown(int button) const -> bool {
    ImGuiIO& io = ImGui::GetIO();
    if (button < GLFW_MOUSE_BUTTON_1 || button > GLFW_MOUSE_BUTTON_LAST || io.WantCaptureMouse) {
        return false;
    }
    return glfwGetMouseButton(window, button) == GLFW_PRESS;
}

void App::initGLFW(bool resizable, const char* title) {
    // Init GLFW
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, resizable);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Create window
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);

    // Set window icon
    GLFWimage icon;
    std::string iconPath = ASSET_DIR + "Vulkan.png";
    icon.pixels = stbi_load(iconPath.c_str(), &icon.width, &icon.height, nullptr, 4);
    if (icon.pixels != nullptr) {
        glfwSetWindowIcon(window, 1, &icon);
    }
    stbi_image_free(icon.pixels);

    // Setup input callbacks
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCharModsCallback(window, charModsCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetCursorEnterCallback(window, cursorEnterCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetDropCallback(window, dropCallback);
    glfwSetWindowSizeCallback(window, windowSizeCallback);
}

void App::initVulkan(ArrayProxy<Layer> requiredLayers, ArrayProxy<Extension> requiredExtensions) {
    bool enableValidation = requiredLayers.contains(Layer::Validation);

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector instanceExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (enableValidation) {
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    std::vector<const char*> layers = {};
    if (enableValidation) {
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }
    if (requiredLayers.contains(Layer::FPSMonitor)) {
        layers.push_back("VK_LAYER_LUNARG_monitor");
    }

    // NOTE: Assuming Vulkan 1.3
    context.initInstance(enableValidation, layers, instanceExtensions, VK_API_VERSION_1_3);

    // Create surface
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(context.getInstance(), window, nullptr, &_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    surface = vk::UniqueSurfaceKHR{_surface, {context.getInstance()}};

    context.initPhysicalDevice(*surface);

    // Create device
    std::vector deviceExtensions{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    if (requiredExtensions.contains(Extension::RayTracing)) {
        deviceExtensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
    }
    if (requiredExtensions.contains(Extension::MeshShader)) {
        deviceExtensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
    }
    if (requiredExtensions.contains(Extension::ShaderObject)) {
        deviceExtensions.push_back(VK_EXT_SHADER_OBJECT_EXTENSION_NAME);
    }

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

    vk::PhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{true};

    vk::PhysicalDeviceShaderObjectFeaturesEXT shaderObjectFeatures{true};

    vk::PhysicalDeviceSynchronization2Features synchronization2Features{true};

    vk::PhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{true};

    vk::PhysicalDeviceHostQueryResetFeaturesEXT hostQueryResetFeatures{true};

    StructureChain featuresChain;
    featuresChain.add(descFeatures);
    featuresChain.add(storage8BitFeatures);
    featuresChain.add(shaderFloat16Int8Features);
    featuresChain.add(bufferDeviceAddressFeatures);
    featuresChain.add(shaderObjectFeatures);
    featuresChain.add(synchronization2Features);
    featuresChain.add(dynamicRenderingFeatures);
    featuresChain.add(hostQueryResetFeatures);

    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{true};
    vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{true};
    vk::PhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{true};
    if (requiredExtensions.contains(Extension::RayTracing)) {
        featuresChain.add(rayTracingPipelineFeatures);
        featuresChain.add(accelerationStructureFeatures);
        featuresChain.add(rayQueryFeatures);
    }

    vk::PhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{true, true};
    if (requiredExtensions.contains(Extension::MeshShader)) {
        featuresChain.add(meshShaderFeatures);
    }

    context.initDevice(deviceExtensions, deviceFeatures, featuresChain.pFirst,
                       requiredExtensions.contains(Extension::RayTracing));

    createSwapchain();
    createDepthImage();

    // Allocate command buffers
    size_t imageCount = swapchainImages.size();
    // Create command buffers and sync objects
    commandBuffers.resize(imageCount);
    fences.resize(imageCount);
    imageAcquiredSemaphores.resize(imageCount);
    renderCompleteSemaphores.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++) {
        commandBuffers[i] = context.allocateCommandBuffer();
        fences[i] = context.createFence({.signaled = true});
        imageAcquiredSemaphores[i] = context.getDevice().createSemaphoreUnique({});
        renderCompleteSemaphores[i] = context.getDevice().createSemaphoreUnique({});
    }
}

void setImGuiStyle() {
    ImVec4* colors = ImGui::GetStyle().Colors;

    // clang-format off
    ImVec4 base = ImVec4(164.0f / 255.0f, 30.0f / 255.0f, 34.0f / 255.0f, 1.0f); // original vulkan theme
    ImVec4 baseLight = ImVec4(202.0f / 255.0f, 36.0f / 255.0f, 41.0f / 255.0f, 1.0f);
    ImVec4 white = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    ImVec4 black = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    ImVec4 gray80 = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    ImVec4 gray60 = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    ImVec4 gray50 = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    ImVec4 gray40 = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    ImVec4 gray30 = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    ImVec4 gray20 = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    ImVec4 gray10 = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);

    colors[ImGuiCol_Text]                  = white;
    colors[ImGuiCol_TextDisabled]          = gray50;
    colors[ImGuiCol_WindowBg]              = gray10;
    colors[ImGuiCol_ChildBg]               = black;
    colors[ImGuiCol_PopupBg]               = gray10;
    colors[ImGuiCol_Border]                = gray20;
    colors[ImGuiCol_BorderShadow]          = black;
    colors[ImGuiCol_FrameBg]               = black;
    colors[ImGuiCol_FrameBgHovered]        = gray20;
    colors[ImGuiCol_FrameBgActive]         = gray20;
    colors[ImGuiCol_TitleBg]               = gray10;
    colors[ImGuiCol_TitleBgActive]         = gray10;
    colors[ImGuiCol_TitleBgCollapsed]      = black;
    colors[ImGuiCol_MenuBarBg]             = gray10;
    colors[ImGuiCol_ScrollbarBg]           = black;
    colors[ImGuiCol_ScrollbarGrab]         = gray30;
    colors[ImGuiCol_ScrollbarGrabHovered]  = gray40;
    colors[ImGuiCol_ScrollbarGrabActive]   = gray50;
    colors[ImGuiCol_CheckMark]             = base;
    colors[ImGuiCol_SliderGrab]            = base;
    colors[ImGuiCol_SliderGrabActive]      = base;
    colors[ImGuiCol_Button]                = base;
    colors[ImGuiCol_ButtonHovered]         = baseLight;
    colors[ImGuiCol_ButtonActive]          = baseLight;
    colors[ImGuiCol_Header]                = base;
    colors[ImGuiCol_HeaderHovered]         = base;
    colors[ImGuiCol_HeaderActive]          = base;
    colors[ImGuiCol_Separator]             = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered]      = base;
    colors[ImGuiCol_SeparatorActive]       = base;
    colors[ImGuiCol_ResizeGrip]            = base;
    colors[ImGuiCol_ResizeGripHovered]     = base;
    colors[ImGuiCol_ResizeGripActive]      = base;
    colors[ImGuiCol_Tab]                   = gray20;
    colors[ImGuiCol_TabHovered]            = gray20;
    colors[ImGuiCol_TabActive]             = gray20;
    colors[ImGuiCol_TabUnfocused]          = gray20;
    colors[ImGuiCol_TabUnfocusedActive]    = gray20;
    colors[ImGuiCol_PlotLines]             = base;
    colors[ImGuiCol_PlotLinesHovered]      = base;
    colors[ImGuiCol_PlotHistogram]         = base;
    colors[ImGuiCol_PlotHistogramHovered]  = base;
    colors[ImGuiCol_TableHeaderBg]         = gray20;
    colors[ImGuiCol_TableBorderStrong]     = gray30;
    colors[ImGuiCol_TableBorderLight]      = gray20;
    colors[ImGuiCol_TableRowBg]            = black;
    colors[ImGuiCol_TableRowBgAlt]         = white;
    colors[ImGuiCol_TextSelectedBg]        = base;
    colors[ImGuiCol_DragDropTarget]        = base;
    colors[ImGuiCol_NavHighlight]          = base;
    colors[ImGuiCol_NavWindowingHighlight] = white;
    colors[ImGuiCol_NavWindowingDimBg]     = gray80;
    colors[ImGuiCol_ModalWindowDimBg]      = gray80;
    // clang-format on
}

void App::initImGui() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    setImGuiStyle();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = context.getInstance();
    initInfo.PhysicalDevice = context.getPhysicalDevice();
    initInfo.Device = context.getDevice();
    initInfo.QueueFamily = context.getQueueFamily();
    initInfo.Queue = context.getQueue();
    initInfo.PipelineCache = nullptr;
    initInfo.DescriptorPool = context.getDescriptorPool();
    initInfo.Subpass = 0;
    initInfo.MinImageCount = minImageCount;
    initInfo.ImageCount = swapchainImages.size();
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.Allocator = nullptr;
    initInfo.UseDynamicRendering = true;
    initInfo.ColorAttachmentFormat = VK_FORMAT_B8G8R8A8_UNORM;
    ImGui_ImplVulkan_Init(&initInfo, {});

    // Setup font
    // TODO: Fix this
    std::string fontFile = ASSET_DIR + "Roboto-Medium.ttf";
    io.Fonts->AddFontFromFileTTF(fontFile.c_str(), 24.0f);
    {
        context.oneTimeSubmit([&](CommandBufferHandle commandBuffer) {
            ImGui_ImplVulkan_CreateFontsTexture(*commandBuffer->commandBuffer);
        });
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
}

void App::createSwapchain() {
    // Create swapchain
    uint32_t queueFamily = context.getQueueFamily();
    swapchain = context.getDevice().createSwapchainKHRUnique(
        vk::SwapchainCreateInfoKHR()
            .setSurface(*surface)
            .setMinImageCount(minImageCount)
            .setImageFormat(vk::Format::eB8G8R8A8Unorm)
            .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
            .setImageExtent({width, height})
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment |
                           vk::ImageUsageFlagBits::eTransferDst)
            .setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
            .setPresentMode(presentMode)
            .setClipped(true)
            .setQueueFamilyIndices(queueFamily));

    // Get images
    swapchainImages = context.getDevice().getSwapchainImagesKHR(*swapchain);

    // Create image views
    for (auto& image : swapchainImages) {
        swapchainImageViews.push_back(context.getDevice().createImageViewUnique(
            vk::ImageViewCreateInfo()
                .setImage(image)
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(vk::Format::eB8G8R8A8Unorm)
                .setComponents({vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
                                vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA})
                .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})));
    }
}

void App::createDepthImage() {
    depthImage = context.createImage({
        .usage = ImageUsage::DepthAttachment,
        .extent = {width, height, 1},
        .format = vk::Format::eD32Sfloat,
        .aspect = vk::ImageAspectFlagBits::eDepth,
        .debugName = "App::depthImage",
    });

    context.oneTimeSubmit([&](CommandBufferHandle commandBuffer) {
        commandBuffer->transitionLayout(depthImage, vk::ImageLayout::eDepthAttachmentOptimal);
    });
}

void App::listSurfaceFormats() {
    auto surfaceFormats = context.getPhysicalDevice().getSurfaceFormatsKHR(*surface);
    spdlog::info("Supported formats:");
    for (const auto& surfaceFormat : surfaceFormats) {
        spdlog::info("  Format: {}, Color Space: {}",  // break
                     vk::to_string(surfaceFormat.format), vk::to_string(surfaceFormat.colorSpace));
    }
}

void App::onWindowSize(int _width, int _height) {
    vk::PhysicalDevice physicalDevice = context.getPhysicalDevice();
    vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
    width = capabilities.currentExtent.width;
    height = capabilities.currentExtent.height;
    if (width == 0 || height == 0) {
        return;
    }
    context.getDevice().waitIdle();

    depthImage.reset();
    swapchainImageViews.clear();
    swapchainImages.clear();
    swapchain.reset();

    createSwapchain();
    createDepthImage();
}

// Callbacks
void App::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard) {
        App* app = (App*)glfwGetWindowUserPointer(window);
        app->onKey(key, scancode, action, mods);
    }
}

void App::charModsCallback(GLFWwindow* window, unsigned int codepoint, int mods) {
    App* app = (App*)glfwGetWindowUserPointer(window);
    app->onCharMods(codepoint, mods);
}

void App::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
        App* app = (App*)glfwGetWindowUserPointer(window);
        app->onMouseButton(button, action, mods);
    }
}

void App::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    App* app = (App*)glfwGetWindowUserPointer(window);
    app->onCursorPos(xpos, ypos);
}

void App::cursorEnterCallback(GLFWwindow* window, int entered) {
    App* app = (App*)glfwGetWindowUserPointer(window);
    app->onCursorEnter(entered);
}

void App::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
        App* app = (App*)glfwGetWindowUserPointer(window);
        app->mouseWheel.x += (float)xoffset;
        app->mouseWheel.y += (float)yoffset;
        app->onScroll(xoffset, yoffset);
    }
}

void App::dropCallback(GLFWwindow* window, int count, const char** paths) {
    App* app = (App*)glfwGetWindowUserPointer(window);
    app->onDrop(count, paths);
}

void App::windowSizeCallback(GLFWwindow* window, int width, int height) {
    App* app = (App*)glfwGetWindowUserPointer(window);
    app->onWindowSize(width, height);
}
}  // namespace rv
