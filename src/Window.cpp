#include "reactive/Window.hpp"

#include <imgui.h>

#include "reactive/App.hpp"

namespace rv {

auto Window::getCursorPos() -> glm::vec2 {
    double xPos{};
    double yPos{};
    glfwGetCursorPos(window, &xPos, &yPos);
    return {xPos, yPos};
}

auto Window::getMouseDragLeft() -> glm::vec2 {
    return mouseDragLeft;
}

auto Window::getMouseDragRight() -> glm::vec2 {
    return mouseDragRight;
}

void Window::processMouseInput() {
    glm::vec2 cursorPos = getCursorPos();
    if (isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)) {
        mouseDragLeft = cursorPos - lastCursorPos;
    } else {
        mouseDragLeft = glm::vec2{0.0f, 0.0f};
    }
    if (isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT)) {
        mouseDragRight = cursorPos - lastCursorPos;
    } else {
        mouseDragRight = glm::vec2{0.0f, 0.0f};
    }
    lastCursorPos = cursorPos;
    mouseScroll = mouseScrollAccum;
    mouseScrollAccum = 0.0f;
}

auto Window::getMouseScroll() -> float {
    return mouseScroll;
}

void Window::setSize(uint32_t _width, uint32_t _height) {
    pendingResize = true;
    newWidth = _width;
    newHeight = _height;
}

bool Window::isKeyDown(int key) {
    if (key < GLFW_KEY_SPACE || key > GLFW_KEY_LAST) {
        return false;
    }
    return glfwGetKey(window, key) == GLFW_PRESS;
}

bool Window::isMouseButtonDown(int button) {
    ImGuiIO& io = ImGui::GetIO();
    if (button < GLFW_MOUSE_BUTTON_1 || button > GLFW_MOUSE_BUTTON_LAST || io.WantCaptureMouse) {
        return false;
    }
    return glfwGetMouseButton(window, button) == GLFW_PRESS;
}

void Window::init(uint32_t _width, uint32_t _height, const char* title, bool resizable) {
    width = _width;
    height = _height;

    // Init GLFW
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, resizable);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Create window
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);

    // Set window icon
    GLFWimage icon;
    std::string iconPath = ASSET_DIR + "Vulkan.png";
    icon.pixels = stbi_load(iconPath.c_str(), &icon.width, &icon.height, nullptr, 4);
    if (icon.pixels != nullptr) {
        glfwSetWindowIcon(window, 1, &icon);
    }
    stbi_image_free(icon.pixels);

    // Setup input callbacks
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCharModsCallback(window, charModsCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetCursorEnterCallback(window, cursorEnterCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetDropCallback(window, dropCallback);
    glfwSetWindowSizeCallback(window, windowSizeCallback);
}

void Window::shutdown() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Window::setAppPointer(App* app) {
    glfwSetWindowUserPointer(window, app);
}

void Window::pollEvents() {
    glfwPollEvents();
    processMouseInput();
    if (pendingResize) {
        glfwSetWindowSize(window, static_cast<int>(newWidth), static_cast<int>(newHeight));
        width = newWidth;
        height = newHeight;
        pendingResize = false;
    }
}

auto Window::createSurface(vk::Instance instance) -> vk::UniqueSurfaceKHR {
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(instance, window, nullptr, &_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
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
void Window::keyCallback(GLFWwindow* _window, int key, int scancode, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard) {
        App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
        app->onKey(key, scancode, action, mods);
    }
}

void Window::charModsCallback(GLFWwindow* _window, unsigned int codepoint, int mods) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    app->onCharMods(codepoint, mods);
}

void Window::mouseButtonCallback(GLFWwindow* _window, int button, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
        if (App* app = static_cast<App*>(glfwGetWindowUserPointer(window))) {
            app->onMouseButton(button, action, mods);
        }
    }
}

void Window::cursorPosCallback(GLFWwindow* _window, double xpos, double ypos) {
    if (App* app = static_cast<App*>(glfwGetWindowUserPointer(window))) {
        app->onCursorPos(static_cast<float>(xpos), static_cast<float>(ypos));
    }
}

void Window::cursorEnterCallback(GLFWwindow* _window, int entered) {
    if (App* app = static_cast<App*>(glfwGetWindowUserPointer(window))) {
        app->onCursorEnter(entered);
    }
}

void Window::scrollCallback(GLFWwindow* _window, double xoffset, double yoffset) {
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
        mouseScrollAccum += static_cast<float>(yoffset);
        if (App* app = static_cast<App*>(glfwGetWindowUserPointer(window))) {
            app->onScroll(static_cast<float>(xoffset), static_cast<float>(yoffset));
        }
    }
}

void Window::dropCallback(GLFWwindow* _window, int count, const char** paths) {
    if (App* app = static_cast<App*>(glfwGetWindowUserPointer(window))) {
        app->onDrop(count, paths);
    }
}

void Window::windowSizeCallback(GLFWwindow* _window, int _width, int _height) {
    width = _width;
    height = _height;
    if (App* app = static_cast<App*>(glfwGetWindowUserPointer(window))) {
        app->onWindowSize();
    }
}
}  // namespace rv
