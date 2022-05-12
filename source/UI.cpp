#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan_hpp.h>
#include "UI.hpp"
#include "Window.hpp"

void UI::Init()
{
    spdlog::info("UI::Init()");

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
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
    initInfo.ImageCount = Vulkan::swapchainImages.size();
    initInfo.MSAASamples = vk::SampleCountFlagBits::e1;
    initInfo.Allocator = nullptr;
    ImGui_ImplVulkan_Init(&initInfo, Vulkan::renderPass);

    // Setup font
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
    Vulkan::BeginRenderPass();
    ImDrawData* drawData = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
    Vulkan::EndRenderPass();
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
