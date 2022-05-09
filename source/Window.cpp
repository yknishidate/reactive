#include "Vulkan/Vulkan.hpp"
#include "Window.hpp"
#include <spdlog/spdlog.h>
#include <stb_image.h>

void Window::Init(int width, int height, const std::string& icon)
{
    spdlog::info("Window::Init()");
    Window::Width = width;
    Window::Height = height;
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, "Window", nullptr, nullptr);
    if (!icon.empty()) {
        SetIcon(icon);
    }
}

void Window::SetIcon(const std::string& filepath)
{
    GLFWimage icon;
    icon.pixels = stbi_load(filepath.c_str(), &icon.width, &icon.height, nullptr, 4);
    if (icon.pixels != nullptr) {
        glfwSetWindowIcon(window, 1, &icon);
    }
    stbi_image_free(icon.pixels);
}

void Window::SetupUI()
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
        RenderPass = Vulkan::Device.createRenderPass(info);
    }

    // Create Framebuffer
    size_t imageCount = Vulkan::SwapchainImages.size();
    Frames.resize(imageCount);
    FrameSemaphores.resize(imageCount);
    {
        vk::ImageView attachment[1];
        vk::FramebufferCreateInfo info{};
        info.renderPass = RenderPass;
        info.attachmentCount = 1;
        info.pAttachments = attachment;
        info.width = Window::GetWidth();
        info.height = Window::GetHeight();
        info.layers = 1;

        for (uint32_t i = 0; i < imageCount; i++) {
            attachment[0] = Vulkan::SwapchainImageViews[i];
            Frames[i].Framebuffer = Vulkan::Device.createFramebuffer(info);
        }
    }

    // Create Command Buffers
    for (uint32_t i = 0; i < imageCount; i++) {
        {
            vk::CommandBufferAllocateInfo commandBufferInfo;
            commandBufferInfo.setCommandPool(Vulkan::CommandPool);
            commandBufferInfo.setLevel(vk::CommandBufferLevel::ePrimary);
            commandBufferInfo.setCommandBufferCount(1);
            Frames[i].CommandBuffer = Vulkan::Device.allocateCommandBuffers(commandBufferInfo)[0];
            //Frames[i].CommandBuffer = Vulkan::AllocateCommandBuffer();
        }
        {
            vk::FenceCreateInfo info{};
            info.flags = vk::FenceCreateFlagBits::eSignaled;
            Frames[i].Fence = Vulkan::Device.createFence(info);
        }
        {
            vk::SemaphoreCreateInfo info{};
            FrameSemaphores[i].ImageAcquiredSemaphore = Vulkan::Device.createSemaphore({});
            FrameSemaphores[i].RenderCompleteSemaphore = Vulkan::Device.createSemaphore({});
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
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = Vulkan::Instance;
    initInfo.PhysicalDevice = Vulkan::PhysicalDevice;
    initInfo.Device = Vulkan::Device;
    initInfo.QueueFamily = Vulkan::QueueFamily;
    initInfo.Queue = Vulkan::Queue;
    initInfo.PipelineCache = nullptr;
    initInfo.DescriptorPool = Vulkan::DescriptorPool;
    initInfo.Subpass = 0;
    initInfo.MinImageCount = minImageCount;
    initInfo.ImageCount = imageCount;
    initInfo.MSAASamples = vk::SampleCountFlagBits::e1;
    initInfo.Allocator = nullptr;
    ImGui_ImplVulkan_Init(&initInfo, RenderPass);

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

int Window::GetWidth()
{
    return Width;
}

int Window::GetHeight()
{
    return Height;
}

vk::CommandBuffer Window::GetCurrentCommandBuffer()
{
    return Frames[FrameIndex].CommandBuffer;
}

vk::Image Window::GetBackImage()
{
    return Vulkan::SwapchainImages[FrameIndex];
}

void Window::Shutdown()
{
    spdlog::info("Window::Shutdown()");
    for (auto&& semaphores : FrameSemaphores) {
        Vulkan::Device.destroySemaphore(semaphores.ImageAcquiredSemaphore);
        Vulkan::Device.destroySemaphore(semaphores.RenderCompleteSemaphore);
    }
    for (auto&& frame : Frames) {
        Vulkan::Device.freeCommandBuffers(Vulkan::CommandPool, frame.CommandBuffer);
        Vulkan::Device.destroyFence(frame.Fence);
        Vulkan::Device.destroyFramebuffer(frame.Framebuffer);
    }
    Vulkan::Device.destroyPipeline(Pipeline);
    Vulkan::Device.destroyRenderPass(RenderPass);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Window::ShouldClose()
{
    return glfwWindowShouldClose(window);
}

void Window::PollEvents()
{
    glfwPollEvents();
}

void Window::RebuildSwapchain()
{
    //int width, height;
    //glfwGetFramebufferSize(window, &width, &height);
    //if (width > 0 && height > 0) {
    //    ImGui_ImplVulkan_SetMinImageCount(minImageCount);
    //    ImGui_ImplVulkanH_CreateOrResizeWindow(Vulkan::Instance, Vulkan::PhysicalDevice, Vulkan::Device, &windowData, Vulkan::QueueFamily, nullptr, width, height, minImageCount);
    //    windowData.FrameIndex = 0;
    //    swapchainRebuild = false;
    //}
}

void Window::StartFrame()
{
    if (swapchainRebuild) {
        RebuildSwapchain();
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

bool Window::IsMinimized()
{
    ImDrawData* drawData = ImGui::GetDrawData();
    return drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f;
}

void Window::WaitNextFrame()
{
    vk::Semaphore imageAcquiredSemaphore = FrameSemaphores[SemaphoreIndex].ImageAcquiredSemaphore;
    vk::Semaphore renderCompleteSemaphore = FrameSemaphores[SemaphoreIndex].RenderCompleteSemaphore;
    try {
        FrameIndex = Vulkan::Device.acquireNextImageKHR(Vulkan::Swapchain, UINT64_MAX, imageAcquiredSemaphore).value;
    } catch (std::exception exception) {
        swapchainRebuild = true;
        return;
    }

    vk::Fence fence = Frames[FrameIndex].Fence;
    Vulkan::Device.waitForFences(fence, VK_TRUE, UINT64_MAX);
    Vulkan::Device.resetFences(fence);
}

void Window::BeginCommandBuffer()
{
    vk::CommandBufferBeginInfo info{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    GetCurrentCommandBuffer().begin(info);
}

void Window::Submit()
{
    vk::CommandBuffer commandBuffer = Frames[FrameIndex].CommandBuffer;
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo info{};
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &FrameSemaphores[SemaphoreIndex].ImageAcquiredSemaphore;
    info.pWaitDstStageMask = &waitStage;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &commandBuffer;
    info.signalSemaphoreCount = 1;
    info.pSignalSemaphores = &FrameSemaphores[SemaphoreIndex].RenderCompleteSemaphore;

    commandBuffer.end();
    vk::Fence fence = Frames[FrameIndex].Fence;
    Vulkan::Queue.submit(info, fence);
}

void Window::RenderUI()
{
    ImDrawData* drawData = ImGui::GetDrawData();

    vk::RenderPassBeginInfo info{};
    info.renderPass = RenderPass;
    info.framebuffer = Frames[FrameIndex].Framebuffer;
    info.renderArea.extent.width = Width;
    info.renderArea.extent.height = Height;
    info.clearValueCount = 1;
    info.pClearValues = &ClearValue;
    Frames[FrameIndex].CommandBuffer.beginRenderPass(info, vk::SubpassContents::eInline);

    ImGui_ImplVulkan_RenderDrawData(drawData, Frames[FrameIndex].CommandBuffer);

    Frames[FrameIndex].CommandBuffer.endRenderPass();
}

void Window::Present()
{
    if (swapchainRebuild) {
        return;
    }
    vk::PresentInfoKHR info{};
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &FrameSemaphores[SemaphoreIndex].RenderCompleteSemaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &Vulkan::Swapchain;
    info.pImageIndices = &FrameIndex;
    try {
        Vulkan::Queue.presentKHR(info);
    } catch (std::exception exception) {
        std::cerr << "failed to present." << std::endl;
        swapchainRebuild = true;
        return;
    }
    SemaphoreIndex = (SemaphoreIndex + 1) % Vulkan::SwapchainImages.size();
}

GLFWwindow* Window::GetWindow()
{
    return window;
}
