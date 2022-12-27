#pragma once
#include <vector>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

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
    static void init(int width, int height);
    static void setIcon(const std::string& filepath);
    static uint32_t getWidth();
    static uint32_t getHeight();
    static GLFWwindow* getWindow();
    static std::vector<const char*> getExtensions();
    static vk::UniqueSurfaceKHR createSurface(vk::Instance instance);

    static void shutdown();
    static bool shouldClose();
    static void pollEvents();
    static bool isMinimized();

    static void update();
    static bool mousePressed();
    static bool keyPressed(int key);
    static glm::vec2 getMousePos() { return currMousePos; }
    static glm::vec2 getMouseMotion() { return currMousePos - lastMousePos; }

private:
    static inline GLFWwindow* window;
    static inline glm::vec2 currMousePos = { 0.0f, 0.0f };
    static inline glm::vec2 lastMousePos = { 0.0f, 0.0f };
};
