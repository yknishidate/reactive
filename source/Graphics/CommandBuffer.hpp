#pragma once
#include <vulkan/vulkan.hpp>

class Image;
class Pipeline;
class ComputePipeline;
class GraphicsPipeline;
class RayTracingPipeline;
class Swapchain;
class Mesh;
class GUI;
class RenderPass;

class CommandBuffer {
public:
    CommandBuffer(Swapchain* swapchain, vk::CommandBuffer commandBuffer)
        : swapchain{swapchain}, commandBuffer{commandBuffer} {}

    void bindPipeline(Pipeline& pipeline);
    void pushConstants(Pipeline& pipeline, void* pushData);

    void traceRays(RayTracingPipeline& rtPipeline, uint32_t countX, uint32_t countY);
    void dispatch(ComputePipeline& compPipeline, uint32_t countX, uint32_t countY);

    void clearBackImage(std::array<float, 4> color);
    void clearColorImage(Image& image, std::array<float, 4> color);
    void clearDepthStencil(Image& image, float depth, uint32_t stencil);
    void beginDefaultRenderPass();
    void beginRenderPass(RenderPass& renderPass);
    void endDefaultRenderPass();
    void endRenderPass(RenderPass& renderPass);
    void submit();
    void drawIndexed(Mesh& mesh);
    void drawGUI(GUI& gui);
    void drawMeshTasks(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
    void copyToBackImage(Image& image);
    void setBackImageLayout(vk::ImageLayout layout);

    vk::CommandBuffer commandBuffer;
    Swapchain* swapchain = nullptr;
};
