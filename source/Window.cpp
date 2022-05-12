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

int Window::GetWidth()
{
    return width;
}

int Window::GetHeight()
{
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
}

bool Window::IsMinimized()
{
    ImDrawData* drawData = ImGui::GetDrawData();
    return drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f;
}

GLFWwindow* Window::GetWindow()
{
    return window;
}
