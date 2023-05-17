#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <vector>

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <spdlog/spdlog.h>
#include <memory>
#include "Graphics/CommandBuffer.hpp"
#include "Graphics/Context.hpp"
#include "Graphics/DescriptorSet.hpp"
#include "Graphics/Image.hpp"
#include "Graphics/Pipeline.hpp"
#include "Graphics/RenderPass.hpp"
#include "Graphics/Shader.hpp"
#include "Scene/AABB.hpp"
#include "Scene/Camera.hpp"
#include "Scene/Loader.hpp"
#include "Scene/Mesh.hpp"
#include "Scene/Object.hpp"
#include "Timer/CPUTimer.hpp"
#include "Timer/GPUTimer.hpp"

namespace File {
template <typename T>
void writeBinary(const std::string& filepath, const std::vector<T>& vec) {
    std::ofstream ofs(filepath, std::ios::out | std::ios::binary);
    if (!ofs) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }
    ofs.write(reinterpret_cast<const char*>(vec.data()), vec.size() * sizeof(T));
    ofs.close();
}

template <typename T>
void readBinary(const std::string& filepath, std::vector<T>& vec) {
    std::uintmax_t size = std::filesystem::file_size(filepath);
    vec.resize(size / sizeof(T));
    std::ifstream ifs(filepath, std::ios::in | std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }
    ifs.read(reinterpret_cast<char*>(vec.data()), vec.size() * sizeof(T));
    ifs.close();
}
}  // namespace File

struct AppCreateInfo {
    // Window
    uint32_t width = 0;
    uint32_t height = 0;
    const char* title = nullptr;
    bool windowResizable = true;

    // Vulkan
    bool enableValidation = true;
    bool enableRayTracing = false;
    bool enableMeshShader = false;
    bool enableShaderObject = false;
};

class App {
public:
    App(AppCreateInfo createInfo);

    void run();

    virtual void onStart() {}
    virtual void onUpdate() {}
    virtual void onRender(const CommandBuffer& commandBuffer) {}

    // void copyToBackImage(vk::CommandBuffer commandBuffer, Image& source) {
    //     vk::ImageCopy copyRegion;
    //     copyRegion.setSrcSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});
    //     copyRegion.setDstSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});
    //     copyRegion.setExtent({width, height, 1});

    //    vk::Image backImage = getCurrentImage();
    //    source.setImageLayout(commandBuffer, vk::ImageLayout::eTransferSrcOptimal);
    //    Image::setImageLayout(commandBuffer, backImage, vk::ImageLayout::eTransferDstOptimal);
    //    commandBuffer.copyImage(source.getImage(), vk::ImageLayout::eTransferSrcOptimal,
    //    backImage,
    //                            vk::ImageLayout::eTransferDstOptimal, copyRegion);
    //    source.setImageLayout(commandBuffer, vk::ImageLayout::eGeneral);
    //    Image::setImageLayout(commandBuffer, backImage, vk::ImageLayout::ePresentSrcKHR);
    //}

    // Getter
    vk::Image getCurrentImage() const { return swapchainImages[frameIndex]; }
    vk::Framebuffer getCurrentFramebuffer() const { return *framebuffers[frameIndex]; }
    vk::RenderPass getDefaultRenderPass() const { return *renderPass; }

    // Input
    bool isKeyDown(int key) const;
    bool isMouseButtonDown(int button) const;
    glm::vec2 getCursorPos() const;
    glm::vec2 getMouseWheel() const { return mouseWheel; }
    virtual void onReset() {}
    virtual void onKey(int key, int scancode, int action, int mods) {}
    virtual void onChar(unsigned int codepoint) {}
    virtual void onCharMods(int codepoint, unsigned int mods) {}
    virtual void onMouseButton(int button, int action, int mods) {}
    virtual void onCursorPos(double xpos, double ypos) {}
    virtual void onCursorEnter(int entered) {}
    virtual void onScroll(double xoffset, double yoffset) {}
    virtual void onDrop(int count, const char** paths) {}
    virtual void onWindowSize(int width, int height) {
        context.getDevice().waitIdle();

        framebuffers.clear();
        depthImageView.reset();
        depthImage.reset();
        depthImageMemory.reset();
        swapchainImageViews.clear();
        swapchainImages.clear();
        swapchain.reset();

        createSwapchain();
        createDepthImage();
        createFramebuffers();
    }

protected:
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void charCallback(GLFWwindow* window, unsigned int codepoint);
    static void charModsCallback(GLFWwindow* window, unsigned int codepoint, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void cursorEnterCallback(GLFWwindow* window, int entered);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void dropCallback(GLFWwindow* window, int count, const char** paths);
    static void windowSizeCallback(GLFWwindow* window, int width, int height);

    void initGLFW(bool resizable, const char* title);
    void initVulkan(bool enableValidation,
                    bool enableRayTracing,
                    bool enableMeshShader,
                    bool enableShaderObject);
    void initImGui();

    void createSwapchain();
    void createDepthImage();
    void createRenderPass();
    void createFramebuffers();

    // GLFW
    GLFWwindow* window;
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

    vk::UniqueImage depthImage;
    vk::UniqueImageView depthImageView;
    vk::UniqueDeviceMemory depthImageMemory;

    bool swapchainRebuild = false;
    int minImageCount = 3;
    uint32_t frameIndex = 0;
    uint32_t imageCount = 0;
    uint32_t semaphoreIndex = 0;

    vk::UniqueRenderPass renderPass;
    vk::UniqueSemaphore imageAcquiredSemaphore;
    vk::UniqueSemaphore renderCompleteSemaphore;
    std::vector<vk::UniqueCommandBuffer> commandBuffers{};
    std::vector<vk::UniqueFramebuffer> framebuffers{};
    std::vector<vk::UniqueFence> fences{};
};
