#include "Window/Window.hpp"
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <imgui.h>

void Window::Init(int width, int height)
{
    spdlog::info("Window::Init()");
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, "Reactive", nullptr, nullptr);
    SetIcon(ASSET_DIR + "Vulkan.png");
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

uint32_t Window::GetWidth()
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    return width;
}

uint32_t Window::GetHeight()
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    return height;
}

void Window::Shutdown()
{
    spdlog::info("Window::Shutdown()");
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

    lastMousePos = currMousePos;
    double xpos{};
    double ypos{};
    glfwGetCursorPos(GetWindow(), &xpos, &ypos);
    currMousePos = { xpos, ypos };
}

bool Window::IsMinimized()
{
    return GetWidth() <= 0 || GetHeight() <= 0;
}

GLFWwindow* Window::GetWindow()
{
    return window;
}

std::vector<const char*> Window::GetExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    return extensions;
}

vk::UniqueSurfaceKHR Window::CreateSurface(vk::Instance instance)
{
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(VkInstance{ instance }, window, nullptr, &_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    return vk::UniqueSurfaceKHR{ _surface,{ instance } };
}

bool Window::MousePressed()
{
    bool pressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    return  pressed && !ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
}

bool Window::MouseRightPressed()
{
    bool pressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    return  pressed && !ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
}

bool Window::KeyPressed(int key)
{
    return glfwGetKey(window, key) == GLFW_PRESS;
}
