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

    CommandBuffer(const Context* context, vk::CommandBuffer commandBuffer, uint32_t queueFlags)
        : context{context},
          queueFlags{queueFlags},
          commandBuffer{commandBuffer,
                        {context->getDevice(), context->getCommandPool(queueFlags)}} {}

    uint32_t getQueueFlags() const { return queueFlags; }

    void begin(vk::CommandBufferUsageFlags flags = {}) const {
        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.setFlags(flags);
        commandBuffer->begin(beginInfo);
    }

    void end() const { commandBuffer->end(); }

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

    void endRendering() const { commandBuffer->endRendering(); }

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
    void pipelineBarrier(
        vk::PipelineStageFlags srcStageMask,
        vk::PipelineStageFlags dstStageMask,
        vk::DependencyFlags dependencyFlags,
        vk::ArrayProxy<const vk::MemoryBarrier> const& memoryBarriers,
        vk::ArrayProxy<const vk::BufferMemoryBarrier> const& bufferMemoryBarriers,
        vk::ArrayProxy<const vk::ImageMemoryBarrier> const& imageMemoryBarriers) const {
        commandBuffer->pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, memoryBarriers,
                                       bufferMemoryBarriers, imageMemoryBarriers);
    }

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
        vk::ArrayProxy<const vk::BufferMemoryBarrier> const& bufferMemoryBarriers) const {
        commandBuffer->pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, nullptr,
                                       bufferMemoryBarriers, nullptr);
    }

    void imageBarrier(
        vk::PipelineStageFlags srcStageMask,
        vk::PipelineStageFlags dstStageMask,
        vk::DependencyFlags dependencyFlags,
        vk::ArrayProxy<const vk::ImageMemoryBarrier> const& imageMemoryBarriers) const {
        commandBuffer->pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, nullptr,
                                       nullptr, imageMemoryBarriers);
    }

    void imageBarrier(vk::PipelineStageFlags srcStageMask,
                      vk::PipelineStageFlags dstStageMask,
                      vk::DependencyFlags dependencyFlags,
                      ImageHandle image,
                      vk::AccessFlags srcAccessMask,
                      vk::AccessFlags dstAccessMask) const;

    void transitionLayout(ImageHandle image, vk::ImageLayout newLayout) const;

    void memoryBarrier(vk::PipelineStageFlags srcStageMask,
                       vk::PipelineStageFlags dstStageMask,
                       vk::DependencyFlags dependencyFlags,
                       vk::ArrayProxy<const vk::MemoryBarrier> const& memoryBarriers) const {
        commandBuffer->pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, memoryBarriers,
                                       nullptr, nullptr);
    }

    void copyImage(ImageHandle srcImage,
                   ImageHandle dstImage,
                   vk::ImageLayout newSrcLayout,
                   vk::ImageLayout newDstLayout) const;

    void copyImageToBuffer(ImageHandle srcImage, BufferHandle dstBuffer) const;
    void copyBufferToImage(BufferHandle srcBuffer, ImageHandle dstImage) const;

    void blitImage(ImageHandle srcImage,
                   ImageHandle dstImage,
                   vk::ImageBlit blit,
                   vk::Filter filter) const {
        commandBuffer->blitImage(srcImage->image, srcImage->layout, dstImage->image,
                                 dstImage->layout, blit, filter);
    }

    void fillBuffer(BufferHandle dstBuffer,
                    vk::DeviceSize dstOffset,
                    vk::DeviceSize size,
                    uint32_t data) const;

    void updateTopAccel(TopAccelHandle topAccel, ArrayProxy<AccelInstance> accelInstances);

    // timestamp
    void beginTimestamp(GPUTimerHandle gpuTimer) const;
    void endTimestamp(GPUTimerHandle gpuTimer) const;

    // dynamic state
    void setLineWidth(float lineWidth) const { commandBuffer->setLineWidth(lineWidth); }
    void setViewport(const vk::Viewport& viewport) const {
        commandBuffer->setViewport(0, 1, &viewport);
    }
    void setViewport(uint32_t width, uint32_t height) const {
        vk::Viewport viewport{
            0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f,
        };
        commandBuffer->setViewport(0, 1, &viewport);
    }
    void setScissor(const vk::Rect2D& scissor) const { commandBuffer->setScissor(0, 1, &scissor); }
    void setScissor(uint32_t width, uint32_t height) const {
        vk::Rect2D scissor{
            {0, 0},
            {width, height},
        };
        commandBuffer->setScissor(0, 1, &scissor);
    }

    void beginDebugLabel(const char* labelName) const {
        if (context->debugEnabled()) {
            vk::DebugUtilsLabelEXT label;
            label.setPLabelName(labelName);
            commandBuffer->beginDebugUtilsLabelEXT(label);
        }
    }

    void endDebugLabel() const {
        if (context->debugEnabled()) {
            commandBuffer->endDebugUtilsLabelEXT();
        }
    }

    const Context* context = nullptr;
    vk::UniqueCommandBuffer commandBuffer;
    uint32_t queueFlags;
};
}  // namespace rv
