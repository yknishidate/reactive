#include "Window/Window.hpp"
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <imgui.h>

void Window::init(int width, int height)
{
    spdlog::info("Window::Init()");
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, "Reactive", nullptr, nullptr);
    setIcon(ASSET_DIR + "Vulkan.png");
}

void Window::setIcon(const std::string& filepath)
{
    GLFWimage icon;
    icon.pixels = stbi_load(filepath.c_str(), &icon.width, &icon.height, nullptr, 4);
    if (icon.pixels != nullptr) {
        glfwSetWindowIcon(window, 1, &icon);
    }
    stbi_image_free(icon.pixels);
}

uint32_t Window::getWidth()
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    return width;
}

uint32_t Window::getHeight()
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    return height;
}

void Window::shutdown()
{
    spdlog::info("Window::Shutdown()");
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Window::shouldClose()
{
    return glfwWindowShouldClose(window);
}

void Window::pollEvents()
{
    glfwPollEvents();

    lastMousePos = currMousePos;
    double xpos{};
    double ypos{};
    glfwGetCursorPos(getWindow(), &xpos, &ypos);
    currMousePos = { xpos, ypos };
}

bool Window::isMinimized()
{
    return getWidth() <= 0 || getHeight() <= 0;
}

GLFWwindow* Window::getWindow()
{
    return window;
}

std::vector<const char*> Window::getExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    return extensions;
}

vk::UniqueSurfaceKHR Window::createSurface(vk::Instance instance)
{
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(VkInstance{ instance }, window, nullptr, &_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    return vk::UniqueSurfaceKHR{ _surface,{ instance } };
}

bool Window::mousePressed()
{
    bool pressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    return  pressed && !ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
}

bool Window::keyPressed(int key)
{
    return glfwGetKey(window, key) == GLFW_PRESS;
}
