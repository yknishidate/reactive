#pragma once
#include "Context.hpp"

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
class DescriptorSet;

class CommandBuffer {
public:
    CommandBuffer(const Context* context, vk::CommandBuffer commandBuffer)
        : context{context}, commandBuffer{commandBuffer} {}

    void bindDescriptorSet(DescriptorSet& descSet, const Pipeline& pipeline) const;
    void bindPipeline(Pipeline& pipeline) const;
    void pushConstants(Pipeline& pipeline, const void* pushData) const;

    void traceRays(const RayTracingPipeline& pipeline,
                   uint32_t countX,
                   uint32_t countY,
                   uint32_t countZ) const;
    void dispatch(const ComputePipeline& pipeline,
                  uint32_t countX,
                  uint32_t countY,
                  uint32_t countZ) const;

    void clearColorImage(vk::Image image, std::array<float, 4> color) const;
    //  void clearColorImage(Image& image, std::array<float, 4> color);

    // render pass
    void beginRenderPass(vk::RenderPass renderPass,
                         vk::Framebuffer framebuffer,
                         uint32_t width,
                         uint32_t height) const;
    void endRenderPass() const { commandBuffer.endRenderPass(); }

    // draw
    void draw(uint32_t vertexCount,
              uint32_t instanceCount,
              uint32_t firstVertex,
              uint32_t firstInstance) const;
    void drawIndexed(const Mesh& mesh, uint32_t instanceCount = 1) const;
    void drawIndexed(const DeviceBuffer& vertexBuffer,
                     const DeviceBuffer& indexBuffer,
                     uint32_t indexCount,
                     uint32_t firstIndex = 0,
                     uint32_t instanceCount = 1,
                     uint32_t firstInstance = 0) const;
    void drawMeshTasks(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) const;

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

    void copyImage(vk::Image srcImage,
                   vk::Image dstImage,
                   vk::ImageLayout newSrcLayout,
                   vk::ImageLayout newDstLayout,
                   uint32_t width,
                   uint32_t height) const;

    // timestamp
    void beginTimestamp(const GPUTimer& gpuTimer) const;
    void endTimestamp(const GPUTimer& gpuTimer) const;

    const Context* context;
    vk::CommandBuffer commandBuffer;
};
