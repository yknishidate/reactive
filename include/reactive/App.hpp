#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <vector>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <memory>
#include "Graphics/CommandBuffer.hpp"
#include "Graphics/Context.hpp"
#include "Graphics/DescriptorSet.hpp"
#include "Graphics/Image.hpp"
#include "Graphics/Pipeline.hpp"
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
};

enum class Layer {
    Validation,
    FPSMonitor,
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
};

class App {
public:
    App(AppCreateInfo createInfo);

    virtual void run();
    virtual void onStart() {}
    virtual void onUpdate() {}
    virtual void onRender(const CommandBufferHandle& commandBuffer) {}

    // Getter
    auto getCurrentColorImage() const -> ImageHandle;
    auto getDefaultDepthImage() const -> ImageHandle;

    // Input
    auto isKeyDown(int key) const -> bool;
    auto isMouseButtonDown(int button) const -> bool;

    auto getCursorPos() const -> glm::vec2;
    auto getMouseWheel() const -> glm::vec2;

    virtual void onReset() {}
    virtual void onKey(int key, int scancode, int action, int mods) {}
    virtual void onChar(unsigned int codepoint) {}
    virtual void onCharMods(int codepoint, unsigned int mods) {}
    virtual void onMouseButton(int button, int action, int mods) {}
    virtual void onCursorPos(double xpos, double ypos) {}
    virtual void onCursorEnter(int entered) {}
    virtual void onScroll(double xoffset, double yoffset) {}
    virtual void onDrop(int count, const char** paths) {}
    virtual void onWindowSize(int width, int height);

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
    void initVulkan(ArrayProxy<Layer> requiredLayers, ArrayProxy<Extension> requiredExtensions);
    void initImGui();

    void createSwapchain();
    void createDepthImage();

    void listSurfaceFormats();

    // GLFW
    GLFWwindow* window = nullptr;
    glm::vec2 mouseWheel{0.0};

    // Context
    Context context;
    vk::UniqueSurfaceKHR surface;

    // Swapchain
    vk::UniqueSwapchainKHR swapchain;
    std::vector<vk::Image> swapchainImages;
    std::vector<vk::UniqueImageView> swapchainImageViews;
    uint32_t width = 0;
    uint32_t height = 0;

    ImageHandle depthImage;

    bool swapchainRebuild = false;
    int minImageCount = 3;
    uint32_t frameIndex = 0;
    uint32_t imageCount = 0;
    uint32_t semaphoreIndex = 0;
    vk::PresentModeKHR presentMode;

    vk::UniqueSemaphore imageAcquiredSemaphore;
    vk::UniqueSemaphore renderCompleteSemaphore;
    std::vector<CommandBufferHandle> commandBuffers{};
    std::vector<FenceHandle> fences{};
};
}  // namespace rv
