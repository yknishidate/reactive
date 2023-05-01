#include "App.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

App::App(uint32_t width, uint32_t height, const std::string& title, bool enableValidation)
    : width{width}, height{height}, title{title}, enableValidation{enableValidation} {
    spdlog::set_pattern("[%^%l%$] %v");

    initGLFW();
    initVulkan();
    initImGui();
}

void App::run() {
    onStart();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Update mouse position
        double xPos{};
        double yPos{};
        glfwGetCursorPos(window, &xPos, &yPos);
        lastMousePos = currMousePos;
        currMousePos = {xPos, yPos};

        onUpdate();

        // Start ImGui
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Acquire next image
        frameIndex = context.getDevice()
                         .acquireNextImageKHR(*swapchain, UINT64_MAX, *imageAcquiredSemaphore)
                         .value;

        // Wait fences
        vk::Result result =
            context.getDevice().waitForFences(*fences[frameIndex], VK_TRUE, UINT64_MAX);
        if (result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to wait for fence");
        }
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

bool App::mousePressed() const {
    bool pressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
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
    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    // Set window icon
    GLFWimage icon;
    std::string iconPath = ASSET_DIR + "Vulkan.png";
    icon.pixels = stbi_load(iconPath.c_str(), &icon.width, &icon.height, nullptr, 4);
    if (icon.pixels != nullptr) {
        glfwSetWindowIcon(window, 1, &icon);
    }
    stbi_image_free(icon.pixels);
}

void App::initVulkan() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector instanceExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    std::vector layers{"VK_LAYER_LUNARG_monitor"};
    if (enableValidation) {
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }

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

    // vk::PhysicalDeviceDescriptorIndexingFeatures descFeatures;
    // descFeatures.setRuntimeDescriptorArray(true);

    // vk::PhysicalDevice8BitStorageFeatures storage8BitFeatures;
    // storage8BitFeatures.setStorageBuffer8BitAccess(true);

    // vk::PhysicalDeviceShaderFloat16Int8Features shaderFloat16Int8Features;
    // shaderFloat16Int8Features.setShaderInt8(true);

    // vk::StructureChain createInfoChain{vk::PhysicalDeviceBufferDeviceAddressFeatures{true},
    //                                    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR{true},
    //                                    vk::PhysicalDeviceAccelerationStructureFeaturesKHR{true},
    //                                    vk::PhysicalDeviceMeshShaderFeaturesEXT{true, true},
    //                                    vk::PhysicalDeviceRayQueryFeaturesKHR{true},
    //                                    descFeatures,
    //                                    shaderFloat16Int8Features,
    //                                    storage8BitFeatures};
    vk::PhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{true};
    context.initDevice(deviceExtensions, deviceFeatures, &bufferDeviceAddressFeatures);

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

    // Create depth image
    depthImage = context.getDevice().createImageUnique(
        vk::ImageCreateInfo()
            .setImageType(vk::ImageType::e2D)
            .setFormat(vk::Format::eD32Sfloat)
            .setExtent({width, height, 1})
            .setMipLevels(1)
            .setArrayLayers(1)
            .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment));

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

    renderPass = context.getDevice().createRenderPassUnique(vk::RenderPassCreateInfo()
                                                                .setAttachments(attachments)
                                                                .setSubpasses(subpass)
                                                                .setDependencies(dependency));

    size_t imageCount = swapchainImages.size();
    commandBuffers = context.allocateCommandBuffers(imageCount);
    framebuffers.resize(imageCount);
    fences.resize(imageCount);
    imageAcquiredSemaphore = context.getDevice().createSemaphoreUnique({});
    renderCompleteSemaphore = context.getDevice().createSemaphoreUnique({});
    for (uint32_t i = 0; i < imageCount; i++) {
        std::array attachments{*swapchainImageViews[i], *depthImageView};
        framebuffers[i] =
            context.getDevice().createFramebufferUnique(vk::FramebufferCreateInfo()
                                                            .setRenderPass(*renderPass)
                                                            .setAttachments(attachments)
                                                            .setWidth(width)
                                                            .setHeight(height)
                                                            .setLayers(1));
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
    std::string fontFile = ASSET_DIR + "Roboto-Medium.ttf";
    io.Fonts->AddFontFromFileTTF(fontFile.c_str(), 24.0f);
    {
        context.oneTimeSubmit([&](vk::CommandBuffer commandBuffer) {
            ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
        });
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
}
