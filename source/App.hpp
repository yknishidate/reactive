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
    bool keyPressed(int key) const { return glfwGetKey(m_window, key) == GLFW_PRESS; }
    glm::vec2 getMousePos() const { return m_currMousePos; }
    glm::vec2 getMouseMotion() const { return m_currMousePos - m_lastMousePos; }

protected:
    void initGLFW();
    void initVulkan();
    void initImGui();

    // GLFW
    GLFWwindow* m_window;
    std::string m_title;
    glm::vec2 m_currMousePos = {0.0f, 0.0f};
    glm::vec2 m_lastMousePos = {0.0f, 0.0f};

    Context context;
    vk::UniqueSurfaceKHR surface;
    bool m_enableValidation;

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
