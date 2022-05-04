#pragma once
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan_hpp.h"
#include <iostream>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include "Image.hpp"
#include "Pipeline.hpp"

struct Frame
{
    vk::CommandBuffer CommandBuffer;
    vk::Fence Fence;
    vk::Framebuffer Framebuffer;
};

struct FrameSemaphores
{
    vk::Semaphore ImageAcquiredSemaphore;
    vk::Semaphore RenderCompleteSemaphore;
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

    static inline int Width;
    static inline int Height;
    static inline vk::RenderPass RenderPass;
    static inline vk::Pipeline Pipeline;
    static inline vk::ClearValue ClearValue;
    static inline uint32_t FrameIndex = 0;
    static inline uint32_t ImageCount = 0;
    static inline uint32_t SemaphoreIndex = 0;
    static inline std::vector<Frame> Frames;
    static inline std::vector<FrameSemaphores> FrameSemaphores;
};
