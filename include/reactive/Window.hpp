#pragma once

#include <GLFW/glfw3.h>

#include "App.hpp"
#include "math.hpp"

namespace rv {
class Window {
public:
    static void init(uint32_t width, uint32_t height, const char* title, bool resizable);

    static void shutdown();

    static void setAppPointer(App* app);

    static bool shouldClose() { return glfwWindowShouldClose(m_window); }

    static void pollEvents();

    static auto getRequiredInstanceExtensions() -> std::vector<const char*>;

    static auto createSurface(vk::Instance instance) -> vk::UniqueSurfaceKHR;

    static bool isKeyDown(int key);
    static bool isMouseButtonDown(int button);

    static void processMouseInput();
    static auto getCursorPos() -> glm::vec2;
    static auto getMouseDragLeft() -> glm::vec2;
    static auto getMouseDragRight() -> glm::vec2;
    static auto getMouseScroll() -> float;
    static void setSize(uint32_t width, uint32_t height);
    static auto getWidth() { return m_width; }
    static auto getHeight() { return m_height; }
    static auto getWindow() { return m_window; }
    static auto getAspect() { return m_width / static_cast<float>(m_height); }

protected:
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void charModsCallback(GLFWwindow* window, unsigned int codepoint, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void cursorEnterCallback(GLFWwindow* window, int entered);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void dropCallback(GLFWwindow* window, int count, const char** paths);
    static void windowSizeCallback(GLFWwindow* window, int width, int height);

    inline static GLFWwindow* m_window = nullptr;
    inline static glm::vec2 m_lastCursorPos{0.0f};
    inline static glm::vec2 m_mouseDragLeft = {0.0f, 0.0f};
    inline static glm::vec2 m_mouseDragRight = {0.0f, 0.0f};

    // GLFW では マウススクロールの絶対値を取得する方法はなく
    // コールバックでオフセットを取得するしかない
    // 1 フレームごとのスクロール量を取るために蓄積する
    inline static float m_mouseScrollAccum = 0.0f;
    inline static float m_mouseScroll = 0.0f;

    inline static bool m_pendingResize = false;
    inline static uint32_t m_width = 0;
    inline static uint32_t m_height = 0;
    inline static uint32_t m_newWidth = 0;
    inline static uint32_t m_newHeight = 0;
};
}  // namespace rv
