#include "reactive/Window.hpp"

#include <imgui.h>

#include "reactive/App.hpp"

namespace rv {

auto Window::getCursorPos() -> glm::vec2 {
    double xPos{};
    double yPos{};
    glfwGetCursorPos(m_window, &xPos, &yPos);
    return {xPos, yPos};
}

auto Window::getMouseDragLeft() -> glm::vec2 {
    return m_mouseDragLeft;
}

auto Window::getMouseDragRight() -> glm::vec2 {
    return m_mouseDragRight;
}

void Window::processMouseInput() {
    glm::vec2 cursorPos = getCursorPos();
    if (isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)) {
        m_mouseDragLeft = cursorPos - m_lastCursorPos;
    } else {
        m_mouseDragLeft = glm::vec2{0.0f, 0.0f};
    }
    if (isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT)) {
        m_mouseDragRight = cursorPos - m_lastCursorPos;
    } else {
        m_mouseDragRight = glm::vec2{0.0f, 0.0f};
    }
    m_lastCursorPos = cursorPos;
    m_mouseScroll = m_mouseScrollAccum;
    m_mouseScrollAccum = 0.0f;
}

auto Window::getMouseScroll() -> float {
    return m_mouseScroll;
}

void Window::setSize(uint32_t _width, uint32_t _height) {
    m_pendingResize = true;
    m_newWidth = _width;
    m_newHeight = _height;
}

bool Window::isKeyDown(int key) {
    if (key < GLFW_KEY_SPACE || key > GLFW_KEY_LAST) {
        return false;
    }
    return glfwGetKey(m_window, key) == GLFW_PRESS;
}

bool Window::isMouseButtonDown(int button) {
    ImGuiIO& io = ImGui::GetIO();
    if (button < GLFW_MOUSE_BUTTON_1 || button > GLFW_MOUSE_BUTTON_LAST || io.WantCaptureMouse) {
        return false;
    }
    return glfwGetMouseButton(m_window, button) == GLFW_PRESS;
}

void Window::init(uint32_t width, uint32_t height, const char* title, bool resizable) {
    m_width = width;
    m_height = height;

    // Init GLFW
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, resizable);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Create window
    m_window = glfwCreateWindow(m_width, m_height, title, nullptr, nullptr);

    // Set window icon
    GLFWimage icon;
    std::string iconPath = ASSET_DIR + "Vulkan.png";
    icon.pixels = stbi_load(iconPath.c_str(), &icon.width, &icon.height, nullptr, 4);
    if (icon.pixels != nullptr) {
        glfwSetWindowIcon(m_window, 1, &icon);
    }
    stbi_image_free(icon.pixels);

    // Setup input callbacks
    glfwSetKeyCallback(m_window, keyCallback);
    glfwSetCharModsCallback(m_window, charModsCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetCursorPosCallback(m_window, cursorPosCallback);
    glfwSetCursorEnterCallback(m_window, cursorEnterCallback);
    glfwSetScrollCallback(m_window, scrollCallback);
    glfwSetDropCallback(m_window, dropCallback);
    glfwSetWindowSizeCallback(m_window, windowSizeCallback);
}

void Window::shutdown() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Window::setAppPointer(App* app) {
    glfwSetWindowUserPointer(m_window, app);
}

void Window::pollEvents() {
    glfwPollEvents();
    processMouseInput();
    if (m_pendingResize) {
        glfwSetWindowSize(m_window, static_cast<int>(m_newWidth), static_cast<int>(m_newHeight));
        m_width = m_newWidth;
        m_height = m_newHeight;
        m_pendingResize = false;
    }
}

auto Window::createSurface(vk::Instance instance) -> vk::UniqueSurfaceKHR {
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(instance, m_window, nullptr, &_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create m_window m_surface!");
    }
    return vk::UniqueSurfaceKHR{_surface, {instance}};
}

auto Window::getRequiredInstanceExtensions() -> std::vector<const char*> {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector instanceExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    return instanceExtensions;
}

// Callbacks
void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard) {
        App* app = static_cast<App*>(glfwGetWindowUserPointer(m_window));
        app->onKey(key, scancode, action, mods);
    }
}

void Window::charModsCallback(GLFWwindow* window, unsigned int codepoint, int mods) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(m_window));
    app->onCharMods(codepoint, mods);
}

void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
        if (App* app = static_cast<App*>(glfwGetWindowUserPointer(m_window))) {
            app->onMouseButton(button, action, mods);
        }
    }
}

void Window::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (App* app = static_cast<App*>(glfwGetWindowUserPointer(m_window))) {
        app->onCursorPos(static_cast<float>(xpos), static_cast<float>(ypos));
    }
}

void Window::cursorEnterCallback(GLFWwindow* window, int entered) {
    if (App* app = static_cast<App*>(glfwGetWindowUserPointer(m_window))) {
        app->onCursorEnter(entered);
    }
}

void Window::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
        m_mouseScrollAccum += static_cast<float>(yoffset);
        if (App* app = static_cast<App*>(glfwGetWindowUserPointer(m_window))) {
            app->onScroll(static_cast<float>(xoffset), static_cast<float>(yoffset));
        }
    }
}

void Window::dropCallback(GLFWwindow* window, int count, const char** paths) {
    if (App* app = static_cast<App*>(glfwGetWindowUserPointer(m_window))) {
        app->onDrop(count, paths);
    }
}

void Window::windowSizeCallback(GLFWwindow* window, int width, int height) {
    m_width = width;
    m_height = height;
    if (App* app = static_cast<App*>(glfwGetWindowUserPointer(m_window))) {
        app->onWindowSize();
    }
}
}  // namespace rv
