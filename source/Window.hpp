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

namespace Key
{
    constexpr inline int W = GLFW_KEY_W;
    constexpr inline int A = GLFW_KEY_A;
    constexpr inline int S = GLFW_KEY_S;
    constexpr inline int D = GLFW_KEY_D;
    constexpr inline int Space = GLFW_KEY_SPACE;
}

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

    static void Update();
    static bool MousePressed();
    static bool KeyPressed(int key);
    static glm::vec2 GetMousePos() { return currMousePos; }
    static glm::vec2 GetMouseMotion() { return currMousePos - lastMousePos; }

private:
    static inline GLFWwindow* window;
    static inline glm::vec2 currMousePos = { 0.0f, 0.0f };
    static inline glm::vec2 lastMousePos = { 0.0f, 0.0f };
};
