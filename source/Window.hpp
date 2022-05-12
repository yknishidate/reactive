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

class Window
{
public:
    static void Init(int width, int height, const std::string& icon = {});
    static void SetIcon(const std::string& filepath);
    static int GetWidth();
    static int GetHeight();

    static void Shutdown();
    static bool ShouldClose();
    static void PollEvents();
    static bool IsMinimized();

    static GLFWwindow* GetWindow();

private:
    static inline GLFWwindow* window;

    static inline int width{};
    static inline int height{};
};
