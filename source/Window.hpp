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
    vk::UniqueCommandBuffer CommandBuffer;
    vk::UniqueFence Fence;
    vk::UniqueFramebuffer Framebuffer;
};

struct FrameSemaphores
{
    vk::UniqueSemaphore ImageAcquiredSemaphore;
    vk::UniqueSemaphore RenderCompleteSemaphore;
};

class Window
{
public:
    static void Init(int width, int height, const std::string& icon = {});

    static void SetIcon(const std::string& filepath);

    static void SetupUI();

    static std::vector<const char*> GetExtensions();

    static vk::SurfaceKHR CreateSurface();

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
    static inline vk::UniqueRenderPass RenderPass;
    static inline vk::UniquePipeline Pipeline;
    static inline bool ClearEnable;
    static inline vk::ClearValue ClearValue;
    static inline uint32_t FrameIndex = 0;
    static inline uint32_t ImageCount = 0;
    static inline uint32_t SemaphoreIndex = 0;
    static inline std::vector<Frame> Frames;
    static inline std::vector<FrameSemaphores> FrameSemaphores;
};
