#pragma once
#include <vulkan/vulkan.hpp>

class App;
class Image;
class Pipeline;
class ComputePipeline;
class GraphicsPipeline;
class RayTracingPipeline;
class Swapchain;
class Mesh;
class RenderPass;
class DeviceBuffer;
class GPUTimer;
class Buffer;

class CommandBuffer {
public:
    CommandBuffer(const App* app, vk::CommandBuffer commandBuffer)
        : m_app{app}, commandBuffer{commandBuffer} {}

    void bindPipeline(Pipeline& pipeline) const;
    void pushConstants(Pipeline& pipeline, const void* pushData) const;

    // void traceRays(RayTracingPipeline& rtPipeline, uint32_t countX, uint32_t countY);
    // void dispatch(ComputePipeline& compPipeline, uint32_t countX, uint32_t countY);

    void clearBackImage(std::array<float, 4> color) const;
    // void clearColorImage(Image& image, std::array<float, 4> color);

    // render pass
    void beginDefaultRenderPass() const;
    // void beginRenderPass(RenderPass& renderPass);
    void endRenderPass() const { commandBuffer.endRenderPass(); }
    // void endRenderPass(RenderPass& renderPass);

    // submit
    // void submit();

    // draw
    void drawIndexed(const Mesh& mesh) const;
    void drawIndexed(const DeviceBuffer& vertexBuffer,
                     const DeviceBuffer& indexBuffer,
                     uint32_t indexCount,
                     uint32_t firstIndex = 0,
                     uint32_t instanceCount = 1,
                     uint32_t firstInstance = 0) const;
    void drawMeshTasks(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

    // barrier
    void pipelineBarrier(
        vk::PipelineStageFlags srcStageMask,
        vk::PipelineStageFlags dstStageMask,
        vk::DependencyFlags dependencyFlags,
        vk::ArrayProxy<const vk::MemoryBarrier> const& memoryBarriers,
        vk::ArrayProxy<const vk::BufferMemoryBarrier> const& bufferMemoryBarriers,
        vk::ArrayProxy<const vk::ImageMemoryBarrier> const& imageMemoryBarriers) const {
        commandBuffer.pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, memoryBarriers,
                                      bufferMemoryBarriers, imageMemoryBarriers);
    }

    void pipelineBarrier(vk::PipelineStageFlags srcStageMask,
                         vk::PipelineStageFlags dstStageMask,
                         vk::DependencyFlags dependencyFlags,
                         const Buffer& buffer,
                         vk::AccessFlags srcAccessMask,
                         vk::AccessFlags dstAccessMask) const;

    void pipelineBarrier(
        vk::PipelineStageFlags srcStageMask,
        vk::PipelineStageFlags dstStageMask,
        vk::DependencyFlags dependencyFlags,
        vk::ArrayProxy<const vk::BufferMemoryBarrier> const& bufferMemoryBarriers) const {
        commandBuffer.pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, nullptr,
                                      bufferMemoryBarriers, nullptr);
    }

    void pipelineBarrier(
        vk::PipelineStageFlags srcStageMask,
        vk::PipelineStageFlags dstStageMask,
        vk::DependencyFlags dependencyFlags,
        vk::ArrayProxy<const vk::ImageMemoryBarrier> const& imageMemoryBarriers) const {
        commandBuffer.pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, nullptr, nullptr,
                                      imageMemoryBarriers);
    }

    void pipelineBarrier(vk::PipelineStageFlags srcStageMask,
                         vk::PipelineStageFlags dstStageMask,
                         vk::DependencyFlags dependencyFlags,
                         vk::ArrayProxy<const vk::MemoryBarrier> const& memoryBarriers) const {
        commandBuffer.pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, memoryBarriers,
                                      nullptr, nullptr);
    }

    // back image
    void copyToBackImage(Image& image);
    void setBackImageLayout(vk::ImageLayout layout);

    // timestamp
    void beginTimestamp(const GPUTimer& gpuTimer) const;
    void endTimestamp(const GPUTimer& gpuTimer) const;

    const App* m_app;
    vk::CommandBuffer commandBuffer;
    // Swapchain* swapchain = nullptr;
};
