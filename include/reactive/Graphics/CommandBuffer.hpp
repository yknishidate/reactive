#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include "Accel.hpp"
#include "Context.hpp"
#include "Image.hpp"

namespace rv {
class App;
class Image;
class Pipeline;
class ComputePipeline;
class GraphicsPipeline;
class RayTracingPipeline;
class Swapchain;
class Mesh;
class RenderPass;
class Buffer;
class GPUTimer;
class Buffer;
class DescriptorSet;

class CommandBuffer {
public:
    CommandBuffer() = default;

    CommandBuffer(const Context* context,
                  vk::CommandBuffer commandBuffer,
                  vk::CommandPool commandPool,
                  vk::QueueFlags queueFlags)
        : context{context},
          commandBuffer{commandBuffer, {context->getDevice(), commandPool}},
          queueFlags{queueFlags} {}

    auto getQueueFlags() const -> vk::QueueFlags;

    void begin(vk::CommandBufferUsageFlags flags = {}) const;
    void end() const;

    void bindDescriptorSet(DescriptorSetHandle descSet, PipelineHandle pipeline) const;
    void bindPipeline(PipelineHandle pipeline) const;
    void pushConstants(PipelineHandle pipeline, const void* pushData) const;

    void bindVertexBuffer(BufferHandle buffer, vk::DeviceSize offset = 0) const;
    void bindIndexBuffer(BufferHandle buffer, vk::DeviceSize offset = 0) const;

    void traceRays(RayTracingPipelineHandle pipeline,
                   uint32_t countX,
                   uint32_t countY,
                   uint32_t countZ) const;

    void dispatch(uint32_t countX, uint32_t countY, uint32_t countZ) const;
    void dispatchIndirect(BufferHandle buffer, vk::DeviceSize offset) const;

    void clearColorImage(ImageHandle image, std::array<float, 4> color) const;
    void clearDepthStencilImage(ImageHandle image, float depth, uint32_t stencil) const;

    void beginRendering(ImageHandle colorImage,
                        ImageHandle depthImage,
                        std::array<int32_t, 2> offset,
                        std::array<uint32_t, 2> extent) const;
    void endRendering() const;

    // draw
    void draw(uint32_t vertexCount,
              uint32_t instanceCount,
              uint32_t firstVertex,
              uint32_t firstInstance) const;
    void drawIndexed(uint32_t indexCount,
                     uint32_t instanceCount = 1,
                     uint32_t firstIndex = 0,
                     int32_t vertexOffset = 0,
                     uint32_t firstInstance = 0) const;
    void drawIndexed(BufferHandle vertexBuffer,
                     BufferHandle indexBuffer,
                     uint32_t indexCount,
                     uint32_t firstIndex = 0,
                     uint32_t instanceCount = 1,
                     uint32_t firstInstance = 0) const;
    void drawMeshTasks(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) const;

    // Indirect draw
    void drawIndirect(BufferHandle buffer,
                      vk::DeviceSize offset = 0,
                      uint32_t drawCount = 1,
                      uint32_t stride = sizeof(vk::DrawIndexedIndirectCommand)) const;
    void drawIndexedIndirect(BufferHandle buffer,
                             vk::DeviceSize offset = 0,
                             uint32_t drawCount = 1,
                             uint32_t stride = sizeof(vk::DrawIndexedIndirectCommand)) const;
    void drawMeshTasksIndirect(BufferHandle buffer,
                               vk::DeviceSize offset,
                               uint32_t drawCount,
                               uint32_t stride) const;

    // barrier
    void bufferBarrier(vk::PipelineStageFlags srcStageMask,
                       vk::PipelineStageFlags dstStageMask,
                       vk::DependencyFlags dependencyFlags,
                       BufferHandle buffer,
                       vk::AccessFlags srcAccessMask,
                       vk::AccessFlags dstAccessMask) const;

    void bufferBarrier(
        vk::PipelineStageFlags srcStageMask,
        vk::PipelineStageFlags dstStageMask,
        vk::DependencyFlags dependencyFlags,
        const vk::ArrayProxy<const vk::BufferMemoryBarrier>& bufferMemoryBarriers) const;

    void imageBarrier(
        vk::PipelineStageFlags srcStageMask,
        vk::PipelineStageFlags dstStageMask,
        vk::DependencyFlags dependencyFlags,
        const vk::ArrayProxy<const vk::ImageMemoryBarrier>& imageMemoryBarriers) const;

    void imageBarrier(vk::PipelineStageFlags srcStageMask,
                      vk::PipelineStageFlags dstStageMask,
                      vk::DependencyFlags dependencyFlags,
                      ImageHandle image,
                      vk::AccessFlags srcAccessMask,
                      vk::AccessFlags dstAccessMask) const;

    void memoryBarrier(vk::PipelineStageFlags srcStageMask,
                       vk::PipelineStageFlags dstStageMask,
                       vk::DependencyFlags dependencyFlags,
                       const vk::ArrayProxy<const vk::MemoryBarrier>& memoryBarriers) const;

    void transitionLayout(ImageHandle image, vk::ImageLayout newLayout) const;

    void copyImage(ImageHandle srcImage,
                   ImageHandle dstImage,
                   vk::ImageLayout newSrcLayout,
                   vk::ImageLayout newDstLayout) const;

    void copyImageToBuffer(ImageHandle srcImage, BufferHandle dstBuffer) const;
    void copyBufferToImage(BufferHandle srcBuffer, ImageHandle dstImage) const;

    void blitImage(ImageHandle srcImage,
                   ImageHandle dstImage,
                   vk::ImageBlit blit,
                   vk::Filter filter) const;

    void fillBuffer(BufferHandle dstBuffer,
                    vk::DeviceSize dstOffset,
                    vk::DeviceSize size,
                    uint32_t data) const;

    // accel struct
    void updateTopAccel(TopAccelHandle topAccel) const;

    void buildTopAccel(TopAccelHandle topAccel) const;

    // timestamp
    void beginTimestamp(GPUTimerHandle gpuTimer) const;
    void endTimestamp(GPUTimerHandle gpuTimer) const;

    // dynamic state
    void setLineWidth(float lineWidth) const;
    void setViewport(const vk::Viewport& viewport) const;
    void setViewport(uint32_t width, uint32_t height) const;
    void setScissor(const vk::Rect2D& scissor) const;
    void setScissor(uint32_t width, uint32_t height) const;

    void beginDebugLabel(const char* labelName) const;
    void endDebugLabel() const;

    const Context* context = nullptr;
    vk::UniqueCommandBuffer commandBuffer;
    vk::QueueFlags queueFlags;
};
}  // namespace rv
