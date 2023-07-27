#include "App.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

struct StructureChain {
    template <typename T>
    void add(T& structure) {
        if (!pFirst) {
            pFirst = &structure;
        } else {
            *ppNext = &structure;
        }
        ppNext = &structure.pNext;
    }

    void* pFirst = nullptr;
    void** ppNext = nullptr;
};

App::App(AppCreateInfo createInfo) : width{createInfo.width}, height{createInfo.height} {
    spdlog::set_pattern("[%^%l%$] %v");

    initGLFW(createInfo.windowResizable, createInfo.title);
    initVulkan(createInfo.enableValidation, createInfo.enableRayTracing,
               createInfo.enableMeshShader, createInfo.enableShaderObject);
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
        vk::Result waitResult =
            context.getDevice().waitForFences(*fences[frameIndex], VK_TRUE, UINT64_MAX);
        if (waitResult != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to wait for fence");
        }

        // Acquire next image
        auto acquireResult = context.getDevice().acquireNextImageKHR(*swapchain, UINT64_MAX,
                                                                     *imageAcquiredSemaphore);
        if (acquireResult.result == vk::Result::eErrorOutOfDateKHR) {
            continue;
        }
        frameIndex = acquireResult.value;

        // Reset fence
        context.getDevice().resetFences(*fences[frameIndex]);

        // Begin command buffer
        commandBuffers[frameIndex]->begin(
            vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        // Render
        CommandBuffer commandBuffer = {&context, *commandBuffers[frameIndex]};
        onRender(commandBuffer);

        // Draw GUI
        {
            // Begin render pass
            commandBuffer.beginRenderPass(*renderPass, *framebuffers[frameIndex], width, height);

            // Render
            ImGui::Render();
            ImDrawData* drawData = ImGui::GetDrawData();
            ImGui_ImplVulkan_RenderDrawData(drawData, *commandBuffers[frameIndex]);

            // End render pass
            commandBuffer.endRenderPass();
        }

        // End command buffer
        commandBuffers[frameIndex]->end();

        // Submit
        vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo submitInfo;
        submitInfo.setWaitDstStageMask(waitStage);
        submitInfo.setCommandBuffers(*commandBuffers[frameIndex]);
        submitInfo.setWaitSemaphores(*imageAcquiredSemaphore);
        submitInfo.setSignalSemaphores(*renderCompleteSemaphore);
        context.getQueue().submit(submitInfo, *fences[frameIndex]);

        // Present image
        vk::PresentInfoKHR presentInfo;
        presentInfo.setWaitSemaphores(*renderCompleteSemaphore);
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

glm::vec2 App::getCursorPos() const {
    double xPos{};
    double yPos{};
    glfwGetCursorPos(window, &xPos, &yPos);
    return {xPos, yPos};
}

bool App::isKeyDown(int key) const {
    ImGuiIO& io = ImGui::GetIO();
    if (key < GLFW_KEY_SPACE || key > GLFW_KEY_LAST || io.WantCaptureKeyboard) {
        return false;
    }
    return glfwGetKey(window, key) == GLFW_PRESS;
}

bool App::isMouseButtonDown(int button) const {
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
    glfwSetCharCallback(window, charCallback);
    glfwSetCharModsCallback(window, charModsCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetCursorEnterCallback(window, cursorEnterCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetDropCallback(window, dropCallback);
    glfwSetWindowSizeCallback(window, windowSizeCallback);
}

void App::initVulkan(bool enableValidation,
                     bool enableRayTracing,
                     bool enableMeshShader,
                     bool enableShaderObject) {
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
    if (enableRayTracing) {
        deviceExtensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
    }
    if (enableMeshShader) {
        deviceExtensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
    }
    if (enableShaderObject) {
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

    StructureChain featuresChain;
    featuresChain.add(descFeatures);
    featuresChain.add(storage8BitFeatures);
    featuresChain.add(shaderFloat16Int8Features);
    featuresChain.add(bufferDeviceAddressFeatures);
    featuresChain.add(shaderObjectFeatures);
    featuresChain.add(synchronization2Features);

    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{true};
    vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{true};
    vk::PhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{true};
    if (enableRayTracing) {
        featuresChain.add(rayTracingPipelineFeatures);
        featuresChain.add(accelerationStructureFeatures);
        featuresChain.add(rayQueryFeatures);
    }

    vk::PhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{true, true};
    if (enableMeshShader) {
        featuresChain.add(meshShaderFeatures);
    }

    context.initDevice(deviceExtensions, deviceFeatures, featuresChain.pFirst, enableRayTracing);

    createSwapchain();
    createDepthImage();
    createRenderPass();
    createFramebuffers();

    // Allocate command buffers
    size_t imageCount = swapchainImages.size();
    commandBuffers = context.allocateCommandBuffers(imageCount);

    // Create sync objects
    fences.resize(imageCount);
    imageAcquiredSemaphore = context.getDevice().createSemaphoreUnique({});
    renderCompleteSemaphore = context.getDevice().createSemaphoreUnique({});
    for (uint32_t i = 0; i < imageCount; i++) {
        fences[i] = context.getDevice().createFenceUnique(
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
    // Vulkan color: RGB(164, 30, 34) or Hex(#A41E22)
    ImVec4 vulkan = ImVec4(164.0f / 255.0f, 30.0f / 255.0f, 34.0f / 255.0f, 1.0f);
    ImVec4 black100 = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    ImVec4 black90 = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    ImVec4 black80 = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);

    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = black90;
    style.Colors[ImGuiCol_TitleBg] = vulkan;
    style.Colors[ImGuiCol_TitleBgActive] = vulkan;
    style.Colors[ImGuiCol_MenuBarBg] = vulkan;
    style.Colors[ImGuiCol_Header] = vulkan;
    style.Colors[ImGuiCol_HeaderActive] = vulkan;
    style.Colors[ImGuiCol_HeaderHovered] = vulkan;
    style.Colors[ImGuiCol_FrameBg] = black100;
    style.Colors[ImGuiCol_FrameBgHovered] = black80;
    style.Colors[ImGuiCol_FrameBgActive] = black80;
    style.Colors[ImGuiCol_CheckMark] = vulkan;
    style.Colors[ImGuiCol_SliderGrab] = vulkan;
    style.Colors[ImGuiCol_SliderGrabActive] = vulkan;
    style.Colors[ImGuiCol_Button] = vulkan;
    style.Colors[ImGuiCol_ButtonHovered] = vulkan;
    style.Colors[ImGuiCol_ButtonActive] = vulkan;
    style.Colors[ImGuiCol_ResizeGrip] = vulkan;
    style.Colors[ImGuiCol_ResizeGripHovered] = vulkan;
    style.Colors[ImGuiCol_ResizeGripActive] = vulkan;

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
    ImGui_ImplVulkan_Init(&initInfo, *renderPass);

    // Setup font
    // TODO: Fix this
    std::string fontFile = ASSET_DIR + "Roboto-Medium.ttf";
    io.Fonts->AddFontFromFileTTF(fontFile.c_str(), 24.0f);
    {
        context.oneTimeSubmit([&](vk::CommandBuffer commandBuffer) {
            ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
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
            .setPresentMode(vk::PresentModeKHR::eFifo)
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
    depthImage = context.getDevice().createImageUnique(
        vk::ImageCreateInfo()
            .setImageType(vk::ImageType::e2D)
            .setFormat(vk::Format::eD32Sfloat)
            .setExtent({width, height, 1})
            .setMipLevels(1)
            .setArrayLayers(1)
            .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment |
                      vk::ImageUsageFlagBits::eTransferDst));

    vk::MemoryRequirements requirements =
        context.getDevice().getImageMemoryRequirements(*depthImage);
    uint32_t memoryTypeIndex =
        context.findMemoryTypeIndex(requirements, vk::MemoryPropertyFlagBits::eDeviceLocal);
    depthImageMemory =
        context.getDevice().allocateMemoryUnique(vk::MemoryAllocateInfo()
                                                     .setAllocationSize(requirements.size)
                                                     .setMemoryTypeIndex(memoryTypeIndex));

    context.getDevice().bindImageMemory(*depthImage, *depthImageMemory, 0);

    depthImageView = context.getDevice().createImageViewUnique(
        vk::ImageViewCreateInfo()
            .setImage(*depthImage)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(vk::Format::eD32Sfloat)
            .setSubresourceRange({vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}));
}

void App::createRenderPass() {
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
    depthAttachment.setLoadOp(vk::AttachmentLoadOp::eDontCare);
    depthAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
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

    renderPass = context.getDevice().createRenderPassUnique(vk::RenderPassCreateInfo()
                                                                .setAttachments(attachments)
                                                                .setSubpasses(subpass)
                                                                .setDependencies(dependency));
}

void App::createFramebuffers() {
    framebuffers.resize(swapchainImages.size());
    for (uint32_t i = 0; i < swapchainImages.size(); i++) {
        std::array attachments{*swapchainImageViews[i], *depthImageView};
        framebuffers[i] =
            context.getDevice().createFramebufferUnique(vk::FramebufferCreateInfo()
                                                            .setRenderPass(*renderPass)
                                                            .setAttachments(attachments)
                                                            .setWidth(width)
                                                            .setHeight(height)
                                                            .setLayers(1));
    }
}

// Callbacks
void App::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard) {
        App* app = (App*)glfwGetWindowUserPointer(window);
        app->onKey(key, scancode, action, mods);
    }
}

void App::charCallback(GLFWwindow* window, unsigned int codepoint) {
    App* app = (App*)glfwGetWindowUserPointer(window);
    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(codepoint);
    app->onChar(codepoint);
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
    app->width = width;
    app->height = height;
    app->onWindowSize(width, height);
}
