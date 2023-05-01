#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <vector>

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

#include "Graphics/CommandBuffer.hpp"
#include "Graphics/Context.hpp"

namespace Key {
constexpr inline int W = GLFW_KEY_W;
constexpr inline int A = GLFW_KEY_A;
constexpr inline int S = GLFW_KEY_S;
constexpr inline int D = GLFW_KEY_D;
constexpr inline int Space = GLFW_KEY_SPACE;
}  // namespace Key

class App {
public:
    App(uint32_t width, uint32_t height, const std::string& title, bool enableValidation);

    void run();

    virtual void onStart() {}
    virtual void onUpdate() {}
    virtual void onRender(const CommandBuffer& commandBuffer) {}

    // void copyToBackImage(vk::CommandBuffer commandBuffer, Image& source) {
    //     vk::ImageCopy copyRegion;
    //     copyRegion.setSrcSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});
    //     copyRegion.setDstSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});
    //     copyRegion.setExtent({width, height, 1});

    //    vk::Image backImage = getBackImage();
    //    source.setImageLayout(commandBuffer, vk::ImageLayout::eTransferSrcOptimal);
    //    Image::setImageLayout(commandBuffer, backImage, vk::ImageLayout::eTransferDstOptimal);
    //    commandBuffer.copyImage(source.getImage(), vk::ImageLayout::eTransferSrcOptimal,
    //    backImage,
    //                            vk::ImageLayout::eTransferDstOptimal, copyRegion);
    //    source.setImageLayout(commandBuffer, vk::ImageLayout::eGeneral);
    //    Image::setImageLayout(commandBuffer, backImage, vk::ImageLayout::ePresentSrcKHR);
    //}

    // Getter
    uint32_t getWidth() const { return width; }
    uint32_t getHeight() const { return height; }
    vk::Image getBackImage() const { return swapchainImages[frameIndex]; }

    // Input
    bool mousePressed() const;
    bool keyPressed(int key) const { return glfwGetKey(window, key) == GLFW_PRESS; }
    glm::vec2 getMousePos() const { return currMousePos; }
    glm::vec2 getMouseMotion() const { return currMousePos - lastMousePos; }

    bool isKeyDown(int key) const;
    bool isMouseButtonDown(int button) const;
    float getMouseWheelH() const { return mouseWheelH; }
    float getMouseWheel() const { return mouseWheel; }
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

    void initGLFW();
    void initVulkan();
    void initImGui();

    void createSwapchain() {
        // Create swapchain
        uint32_t queueFamily = context.getQueueFamily();
        swapchain = context.getDevice().createSwapchainKHRUnique(
            vk::SwapchainCreateInfoKHR()
                .setSurface(*surface)
                .setMinImageCount(minImageCount)
                .setImageFormat(vk::Format::eB8G8R8A8Unorm)
                .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
                .setImageExtent({width, height})
                .setImageArrayLayers(1)
                .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment |
                               vk::ImageUsageFlagBits::eTransferDst)
                .setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
                .setPresentMode(vk::PresentModeKHR::eFifo)
                .setClipped(true)
                .setQueueFamilyIndices(queueFamily));

        // Get images
        swapchainImages = context.getDevice().getSwapchainImagesKHR(*swapchain);

        // Create image views
        for (auto& image : swapchainImages) {
            swapchainImageViews.push_back(context.getDevice().createImageViewUnique(
                vk::ImageViewCreateInfo()
                    .setImage(image)
                    .setViewType(vk::ImageViewType::e2D)
                    .setFormat(vk::Format::eB8G8R8A8Unorm)
                    .setComponents({vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
                                    vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA})
                    .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})));
        }
    }

    void createDepthImage() {
        depthImage = context.getDevice().createImageUnique(
            vk::ImageCreateInfo()
                .setImageType(vk::ImageType::e2D)
                .setFormat(vk::Format::eD32Sfloat)
                .setExtent({width, height, 1})
                .setMipLevels(1)
                .setArrayLayers(1)
                .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment));

        vk::MemoryRequirements requirements =
            context.getDevice().getImageMemoryRequirements(*depthImage);
        uint32_t memoryTypeIndex =
            context.findMemoryTypeIndex(requirements, vk::MemoryPropertyFlagBits::eDeviceLocal);
        depthImageMemory =
            context.getDevice().allocateMemoryUnique(vk::MemoryAllocateInfo()
                                                         .setAllocationSize(requirements.size)
                                                         .setMemoryTypeIndex(memoryTypeIndex));

        context.getDevice().bindImageMemory(*depthImage, *depthImageMemory, 0);

        depthImageView = context.getDevice().createImageViewUnique(
            vk::ImageViewCreateInfo()
                .setImage(*depthImage)
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(vk::Format::eD32Sfloat)
                .setSubresourceRange({vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}));
    }

    void createFramebuffers() {
        framebuffers.resize(swapchainImages.size());
        for (uint32_t i = 0; i < swapchainImages.size(); i++) {
            std::array attachments{*swapchainImageViews[i], *depthImageView};
            framebuffers[i] =
                context.getDevice().createFramebufferUnique(vk::FramebufferCreateInfo()
                                                                .setRenderPass(*renderPass)
                                                                .setAttachments(attachments)
                                                                .setWidth(width)
                                                                .setHeight(height)
                                                                .setLayers(1));
        }
    }

    // GLFW
    GLFWwindow* window;
    std::string title;
    float mouseWheelH = 0.0f;
    float mouseWheel = 0.0f;
    glm::vec2 currMousePos = {0.0f, 0.0f};
    glm::vec2 lastMousePos = {0.0f, 0.0f};

    Context context;
    vk::UniqueSurfaceKHR surface;
    bool enableValidation;

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
