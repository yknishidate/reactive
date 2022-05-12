#pragma once
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan_hpp.h"
#include <iostream>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include "Vulkan/Image.hpp"
#include "Vulkan/Pipeline.hpp"

struct Frame
{
    vk::CommandBuffer commandBuffer{};
    vk::Fence fence{};
    vk::Framebuffer framebuffer{};
};

struct FrameSemaphores
{
    vk::Semaphore imageAcquiredSemaphore{};
    vk::Semaphore renderCompleteSemaphore{};
};

class Window
{
public:
    static void Init(int width, int height, const std::string& icon = {});
    static void SetIcon(const std::string& filepath);
    static void SetupUI();
    static int GetWidth();
    static int GetHeight();

    static vk::CommandBuffer GetCurrentCommandBuffer();
    static vk::Image GetBackImage();

    static void Shutdown();
    static bool ShouldClose();
    static void PollEvents();
    static void RebuildSwapchain();
    static void StartFrame();
    static bool IsMinimized();
    static void WaitNextFrame();
    static void BeginCommandBuffer();
    static void Submit();
    static void RenderUI();
    static void Present();

    static GLFWwindow* GetWindow();

private:
    static inline GLFWwindow* window;
    static inline int minImageCount = 3;
    static inline bool swapchainRebuild = false;

    static inline int width{};
    static inline int height{};
    static inline vk::RenderPass renderPass{};
    static inline vk::Pipeline pipeline{};
    static inline vk::ClearValue clearValue{};
    static inline uint32_t frameIndex = 0;
    static inline uint32_t imageCount = 0;
    static inline uint32_t semaphoreIndex = 0;
    static inline std::vector<Frame> frames{};
    static inline std::vector<FrameSemaphores> frameSemaphores{};
};
