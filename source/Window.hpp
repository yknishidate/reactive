#pragma once
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "ImGui/imgui_impl_vulkan_hpp.h"
#include <iostream>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include "Vulkan/Image.hpp"
#include "Vulkan/Pipeline.hpp"

class Window
{
public:
    static void Init(int width, int height);
    static void SetIcon(const std::string& filepath);
    static uint32_t GetWidth();
    static uint32_t GetHeight();
    static GLFWwindow* GetWindow();
    static std::vector<const char*> GetExtensions();
    static vk::UniqueSurfaceKHR CreateSurface(vk::Instance instance);

    static void Shutdown();
    static bool ShouldClose();
    static void PollEvents();
    static bool IsMinimized();

private:
    static inline GLFWwindow* window;
};
