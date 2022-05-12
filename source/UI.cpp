#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan_hpp.h>
#include "UI.hpp"
#include "Window.hpp"

void UI::Init()
{
    spdlog::info("UI::Init()");

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
        renderPass = Vulkan::device.createRenderPassUnique(info);
    }

    // Create Framebuffer
    size_t imageCount = Vulkan::swapchainImages.size();
    framebuffers.resize(imageCount);
    {
        vk::ImageView attachment[1];
        vk::FramebufferCreateInfo info{};
        info.renderPass = *renderPass;
        info.attachmentCount = 1;
        info.pAttachments = attachment;
        info.width = Window::GetWidth();
        info.height = Window::GetHeight();
        info.layers = 1;

        for (uint32_t i = 0; i < imageCount; i++) {
            attachment[0] = Vulkan::swapchainImageViews[i];
            framebuffers[i] = Vulkan::device.createFramebufferUnique(info);
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
    initInfo.MinImageCount = Vulkan::minImageCount;
    initInfo.ImageCount = imageCount;
    initInfo.MSAASamples = vk::SampleCountFlagBits::e1;
    initInfo.Allocator = nullptr;
    ImGui_ImplVulkan_Init(&initInfo, *renderPass);

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

void UI::StartFrame()
{
    //if (swapchainRebuild) {
    //    RebuildSwapchain();
    //}
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UI::Prepare()
{
    ImGui::Render();
}

void UI::Render(vk::CommandBuffer commandBuffer)
{
    ImDrawData* drawData = ImGui::GetDrawData();

    vk::RenderPassBeginInfo info{};
    info.renderPass = *renderPass;
    info.framebuffer = *framebuffers[Vulkan::GetCurrentImageIndex()];
    info.renderArea.extent.width = Window::GetWidth();
    info.renderArea.extent.height = Window::GetHeight();
    info.clearValueCount = 1;
    info.pClearValues = &clearValue;
    commandBuffer.beginRenderPass(info, vk::SubpassContents::eInline);

    ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);

    commandBuffer.endRenderPass();
}

bool UI::Checkbox(const std::string& label, bool& value)
{
    return ImGui::Checkbox(label.c_str(), &value);
}

bool UI::Combo(const std::string& label, int& value, const std::vector<std::string>& items)
{
    std::string concats;
    for (auto&& item : items) {
        concats += item + '\0';
    }
    return ImGui::Combo(label.c_str(), &value, concats.c_str());
}

bool UI::SliderInt(const std::string& label, int& value, int min, int max)
{
    return ImGui::SliderInt(label.c_str(), &value, min, max);
}

bool UI::ColorPicker4(const std::string& label, glm::vec4& value)
{
    return ImGui::ColorPicker4(label.c_str(), reinterpret_cast<float*>(&value));
}
