#include "reactive/App.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#include "reactive/Window.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace rv {

App::App(const AppCreateInfo& createInfo) {
    spdlog::set_pattern("[%^%l%$] %v");

    Window::init(createInfo.width, createInfo.height, createInfo.title, createInfo.windowResizable);
    Window::setAppPointer(this);
    initVulkan(createInfo.layers, createInfo.extensions, createInfo.vsync);
    initImGui(createInfo.style, createInfo.imguiIniFile);
}

void App::run() {
    onStart();
    CPUTimer timer;

    while (!Window::shouldClose() && m_running) {
        Window::pollEvents();

        if (Window::getWidth() == 0 && Window::getHeight() == 0) {
            continue;
        }

        // Start ImGui
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        float dt = timer.elapsedInMilli();
        onUpdate(dt);
        timer.restart();

        m_swapchain->waitNextFrame();

        // Begin command buffer
        // NOTE: Since the command pool is created with the Reset flag,
        //       the command buffer is implicitly reset at begin.
        auto commandBuffer = m_swapchain->getCurrentCommandBuffer();
        commandBuffer->begin();

        // Render
        onRender(commandBuffer);

        // Draw GUI
        {
            // Begin render pass
            commandBuffer->beginDebugLabel("ImGui");
            commandBuffer->beginRendering(getCurrentColorImage(), {}, {0, 0},
                                          {Window::getWidth(), Window::getHeight()});

            // Render
            // TODO: create ImGui wrapper
            ImGui::Render();
            ImDrawData* drawData = ImGui::GetDrawData();
            ImGui_ImplVulkan_RenderDrawData(drawData, *commandBuffer->m_commandBuffer);

            // End render pass
            commandBuffer->endRendering();
            commandBuffer->endDebugLabel();
        }

        commandBuffer->transitionLayout(getCurrentColorImage(), vk::ImageLayout::ePresentSrcKHR);

        // End command buffer
        commandBuffer->end();

        // Submit
        vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        m_context.submit(commandBuffer, waitStage, m_swapchain->getCurrentImageAcquiredSemaphore(),
                       m_swapchain->getCurrentRenderCompleteSemaphore(),
                       m_swapchain->getCurrentFence());

        // Present image
        m_swapchain->presentImage();
    }
    m_context.getDevice().waitIdle();

    Window::shutdown();

    // Shutdown ImGui
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    onShutdown();
}

auto App::getCurrentColorImage() const -> ImageHandle {
    return std::make_shared<Image>(m_swapchain->getCurrentImage(), m_swapchain->getCurrentImageView(),
                                   vk::Extent3D{Window::getWidth(), Window::getHeight(), 1},
                                   m_swapchain->getFormat(), vk::ImageAspectFlagBits::eColor);
}

void App::initVulkan(ArrayProxy<Layer> requiredLayers,
                     ArrayProxy<Extension> requiredExtensions,
                     bool vsync) {
    bool enableValidation = requiredLayers.contains(Layer::Validation);

    std::vector instanceExtensions = Window::getRequiredInstanceExtensions();
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
    m_context.initInstance(enableValidation, layers, instanceExtensions, VK_API_VERSION_1_3);

    // Create surface
    m_surface = Window::createSurface(m_context.getInstance());

    m_context.initPhysicalDevice(*m_surface);

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
    if (requiredExtensions.contains(Extension::DeviceFault)) {
        deviceExtensions.push_back(VK_EXT_DEVICE_FAULT_EXTENSION_NAME);
    }
    if (requiredExtensions.contains(Extension::ExtendedDynamicState)) {
        deviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
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

    vk::PhysicalDeviceScalarBlockLayoutFeatures scalarBlockLayoutFeatures{true};

    vk::PhysicalDeviceSynchronization2Features synchronization2Features{true};

    vk::PhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{true};

    vk::PhysicalDeviceHostQueryResetFeatures hostQueryResetFeatures{true};

    StructureChain featuresChain;
    featuresChain.add(descFeatures);
    featuresChain.add(storage8BitFeatures);
    featuresChain.add(shaderFloat16Int8Features);
    featuresChain.add(bufferDeviceAddressFeatures);
    featuresChain.add(scalarBlockLayoutFeatures);
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

    vk::PhysicalDeviceShaderObjectFeaturesEXT shaderObjectFeatures{true};
    if (requiredExtensions.contains(Extension::ShaderObject)) {
        featuresChain.add(shaderObjectFeatures);
    }

    vk::PhysicalDeviceFaultFeaturesEXT faultFeatures{true, true};
    if (requiredExtensions.contains(Extension::DeviceFault)) {
        featuresChain.add(faultFeatures);
    }

    vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT extendedDynamicState3Features{};
    extendedDynamicState3Features.setExtendedDynamicState3PolygonMode(true);
    if (requiredExtensions.contains(Extension::ExtendedDynamicState)) {
        featuresChain.add(extendedDynamicState3Features);
    }

    m_context.initDevice(deviceExtensions, deviceFeatures, featuresChain.pFirst,
                       requiredExtensions.contains(Extension::RayTracing));

    auto presentMode = vsync ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eMailbox;
    m_swapchain = std::make_unique<Swapchain>(m_context, *m_surface, Window::getWidth(),
                                            Window::getHeight(), presentMode);
}

void setImGuiStyle(UIStyle style) {
    if (style == UIStyle::ImGui) {
        return;
    }

    ImVec4* colors = ImGui::GetStyle().Colors;

    // clang-format off
    ImVec4 white = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    ImVec4 black = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    ImVec4 gray80 = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    ImVec4 gray50 = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    ImVec4 gray40 = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    ImVec4 gray30 = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    ImVec4 gray20 = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    ImVec4 gray10 = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    ImVec4 base;
    ImVec4 baseLight;
    if(style == UIStyle::Vulkan) {
        base = ImVec4(164.0f / 255.0f, 30.0f / 255.0f, 34.0f / 255.0f, 1.0f); // original vulkan theme
        baseLight = ImVec4(202.0f / 255.0f, 36.0f / 255.0f, 41.0f / 255.0f, 1.0f);
    }else if(style == UIStyle::Gray) {
        base = gray30;
        baseLight = gray80;
    }

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

void App::initImGui(UIStyle style, const char* imguiIniFile) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    if (imguiIniFile) {
        io.IniFilename = imguiIniFile;
    }
    setImGuiStyle(style);

    // Setup Platform/Renderer backends
    vk::Format colorFormat = vk::Format::eB8G8R8A8Unorm;
    vk::PipelineRenderingCreateInfo renderingCreateInfo;
    renderingCreateInfo.setColorAttachmentFormats(colorFormat);

    ImGui_ImplGlfw_InitForVulkan(Window::getWindow(), true);
    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = m_context.getInstance();
    initInfo.PhysicalDevice = m_context.getPhysicalDevice();
    initInfo.Device = m_context.getDevice();
    initInfo.QueueFamily = m_context.getQueueFamily();
    initInfo.Queue = m_context.getQueue();
    initInfo.PipelineCache = nullptr;
    initInfo.DescriptorPool = m_context.getDescriptorPool();
    initInfo.Subpass = 0;
    initInfo.MinImageCount = m_swapchain->getMinImageCount();
    initInfo.ImageCount = m_swapchain->getImageCount();
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.Allocator = nullptr;
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineRenderingCreateInfo = renderingCreateInfo;
    ImGui_ImplVulkan_Init(&initInfo);

    // Setup font
    std::string fontFile = ASSET_DIR + "Roboto-Medium.ttf";
    io.Fonts->AddFontFromFileTTF(fontFile.c_str(), 24.0f);
}

void App::listSurfaceFormats() {
    auto surfaceFormats = m_context.getPhysicalDevice().getSurfaceFormatsKHR(*m_surface);
    spdlog::info("Supported formats:");
    for (const auto& surfaceFormat : surfaceFormats) {
        spdlog::info("  Format: {}", vk::to_string(surfaceFormat.format));
        spdlog::info("  Color Space: {}", vk::to_string(surfaceFormat.colorSpace));
    }
}

void App::onWindowSize() {
    // NOTE:
    // The value obtained from GLFW and the value obtained by
    // getSurfaceCapabilitiesKHR() should be the same,
    // but a validation error occurs if the Vulkan function is not called.
    vk::PhysicalDevice physicalDevice = m_context.getPhysicalDevice();
    vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR(*m_surface);
    uint32_t width = capabilities.currentExtent.width;
    uint32_t height = capabilities.currentExtent.height;
    spdlog::debug("Window resized: {} {}", width, height);
    if (width == 0 || height == 0) {
        return;
    }
    m_context.getDevice().waitIdle();
    m_swapchain->resize(width, height);
}
}  // namespace rv
