#pragma once
#include <vulkan/vulkan.hpp>

class Image;
class Pipeline;
class GraphicsPipeline;
class Swapchain;
class Mesh;
class GUI;

class CommandBuffer
{
public:
    CommandBuffer(Swapchain* swapchain, vk::CommandBuffer commandBuffer)
        : swapchain{ swapchain }
        , commandBuffer{ commandBuffer }
    {}

    void bindPipeline(GraphicsPipeline& pipeline);
    void pushConstants(void* pushData);
    void clearBackImage(std::array<float, 4> color);
    void beginRenderPass();
    void endRenderPass();
    void submit();
    void drawIndexed(Mesh& mesh);
    void drawGUI(GUI& gui);

    vk::CommandBuffer commandBuffer;
    Pipeline* boundPipeline = nullptr;
    Swapchain* swapchain = nullptr;
};
