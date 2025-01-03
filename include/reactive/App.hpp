#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

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
#include "Timer/CPUTimer.hpp"

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

    // UI
    UIStyle style = UIStyle::Vulkan;
    const char* imguiIniFile = nullptr;
};

class App {
public:
    App(const AppCreateInfo& createInfo);
    virtual ~App() {}

    virtual void run();
    virtual void onStart() {}
    virtual void onUpdate(float dt) {}
    virtual void onRender(const CommandBufferHandle& commandBuffer) {}
    virtual void onShutdown() {}

    void terminate() { m_running = false; }

    // Getter
    auto getCurrentColorImage() const -> ImageHandle;

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
    void initVulkan(ArrayProxy<Layer> requiredLayers,
                    ArrayProxy<Extension> requiredExtensions,
                    bool vsync);

    void initImGui(UIStyle style, const char* imguiIniFile);

    void listSurfaceFormats();

    Context m_context;
    vk::UniqueSurfaceKHR m_surface;
    std::unique_ptr<Swapchain> m_swapchain;
    bool m_running = true;
};
}  // namespace rv
