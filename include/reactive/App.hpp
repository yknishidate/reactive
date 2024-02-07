#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <memory>
#include "Graphics/CommandBuffer.hpp"
#include "Graphics/Context.hpp"
#include "Graphics/DescriptorSet.hpp"
#include "Graphics/Image.hpp"
#include "Graphics/Pipeline.hpp"
#include "Graphics/Swapchain.hpp"
#include "Scene/Loader.hpp"
#include "Scene/Mesh.hpp"
#include "Scene/Object.hpp"

namespace rv {
struct StructureChain {
    template <typename T>
    void add(T& structure) {
        if (!pFirst) {
            pFirst = &structure;
        } else {
            *ppNext = &structure;
        }
        ppNext = &structure.pNext;
    }

    void* pFirst = nullptr;
    void** ppNext = nullptr;
};

enum class Extension {
    RayTracing,
    MeshShader,
    ShaderObject,
    DeviceFault,
    ExtendedDynamicState,
};

enum class Layer {
    Validation,
    FPSMonitor,
};

enum class UIStyle {
    ImGui,
    Vulkan,
    Gray,
};

struct AppCreateInfo {
    // Window
    uint32_t width = 0;
    uint32_t height = 0;
    const char* title = nullptr;
    bool windowResizable = true;
    bool vsync = true;

    // Vulkan
    ArrayProxy<Layer> layers;
    ArrayProxy<Extension> extensions;

    UIStyle style = UIStyle::Vulkan;
};

class App {
public:
    App(AppCreateInfo createInfo);

    virtual void run();
    virtual void onStart() {}
    virtual void onUpdate() {}
    virtual void onRender(const CommandBufferHandle& commandBuffer) {}
    virtual void onShutdown() {}

    void terminate() { running = false; }

    // Getter
    auto getCurrentColorImage() const -> ImageHandle;

    // Input
    auto isKeyDown(int key) const -> bool;
    auto isMouseButtonDown(int button) const -> bool;

    void processMouseInput();
    auto getCursorPos() const -> glm::vec2;
    auto getMouseDrag() const -> glm::vec2;
    auto getMouseScroll() const -> float;
    void setWindowSize(uint32_t _width, uint32_t _height);

    virtual void onReset() {}
    virtual void onKey(int key, int scancode, int action, int mods) {}
    virtual void onChar(unsigned int codepoint) {}
    virtual void onCharMods(int codepoint, unsigned int mods) {}
    virtual void onMouseButton(int button, int action, int mods) {}
    virtual void onCursorPos(float xpos, float ypos) {}
    virtual void onCursorEnter(int entered) {}
    virtual void onScroll(float xoffset, float yoffset) {}
    virtual void onDrop(int count, const char** paths) {}
    virtual void onWindowSize();

protected:
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void charModsCallback(GLFWwindow* window, unsigned int codepoint, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void cursorEnterCallback(GLFWwindow* window, int entered);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void dropCallback(GLFWwindow* window, int count, const char** paths);
    static void windowSizeCallback(GLFWwindow* window, int width, int height);

    void initGLFW(bool resizable, const char* title);

    void initVulkan(ArrayProxy<Layer> requiredLayers,
                    ArrayProxy<Extension> requiredExtensions,
                    bool vsync);

    void initImGui(UIStyle style);

    void listSurfaceFormats();

    GLFWwindow* window = nullptr;
    glm::vec2 lastCursorPos{0.0f};
    glm::vec2 mouseDrag = {0.0f, 0.0f};

    // GLFW では マウススクロールの絶対値を取得する方法はなく
    // コールバックでオフセットを取得するしかない
    // 1 フレームごとのスクロール量を取るために蓄積する
    float mouseScrollAccum = 0.0f;
    float mouseScroll = 0.0f;

    Context context;
    vk::UniqueSurfaceKHR surface;
    std::unique_ptr<Swapchain> swapchain;
    uint32_t width = 0;
    uint32_t height = 0;
    bool running = true;

    bool pendingResize = false;
    uint32_t newWidth = 0;
    uint32_t newHeight = 0;
};
}  // namespace rv
