#include "Vulkan/Vulkan.hpp"
#include "Window.hpp"
#include <spdlog/spdlog.h>
#include <stb_image.h>

void Window::Init(int width, int height, const std::string& icon)
{
    spdlog::info("Window::Init()");
    Window::width = width;
    Window::height = height;
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, "Reactive", nullptr, nullptr);
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
    ImGui_ImplGlfw_InitForVulkan(window, true);
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

int Window::GetWidth()
{
    return width;
}

int Window::GetHeight()
{
    return height;
}

vk::CommandBuffer Window::GetCurrentCommandBuffer()
{
    return frames[frameIndex].commandBuffer;
}

vk::Image Window::GetBackImage()
{
    return Vulkan::swapchainImages[frameIndex];
}

void Window::Shutdown()
{
    spdlog::info("Window::Shutdown()");
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
    //    ImGui_ImplVulkanH_CreateOrResizeWindow(Vulkan::instance, Vulkan::physicalDevice, Vulkan::device, &windowData, Vulkan::queueFamily, nullptr, width, height, minImageCount);
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

void Window::BeginCommandBuffer()
{
    vk::CommandBufferBeginInfo info{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    GetCurrentCommandBuffer().begin(info);
}

void Window::Submit()
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

void Window::RenderUI()
{
    ImDrawData* drawData = ImGui::GetDrawData();

    vk::RenderPassBeginInfo info{};
    info.renderPass = renderPass;
    info.framebuffer = frames[frameIndex].framebuffer;
    info.renderArea.extent.width = width;
    info.renderArea.extent.height = height;
    info.clearValueCount = 1;
    info.pClearValues = &clearValue;
    frames[frameIndex].commandBuffer.beginRenderPass(info, vk::SubpassContents::eInline);

    ImGui_ImplVulkan_RenderDrawData(drawData, frames[frameIndex].commandBuffer);

    frames[frameIndex].commandBuffer.endRenderPass();
}

void Window::Present()
{
    if (swapchainRebuild) {
        return;
    }
    vk::PresentInfoKHR info{};
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &frameSemaphores[semaphoreIndex].renderCompleteSemaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &Vulkan::swapchain;
    info.pImageIndices = &frameIndex;
    try {
        Vulkan::queue.presentKHR(info);
    } catch (std::exception exception) {
        std::cerr << "failed to present." << std::endl;
        swapchainRebuild = true;
        return;
    }
    semaphoreIndex = (semaphoreIndex + 1) % Vulkan::swapchainImages.size();
}

GLFWwindow* Window::GetWindow()
{
    return window;
}
