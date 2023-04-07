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
class DeviceBuffer;
class GPUTimer;
class Buffer;

class CommandBuffer {
public:
    CommandBuffer(Swapchain* swapchain, vk::CommandBuffer commandBuffer)
        : swapchain{swapchain}, commandBuffer{commandBuffer} {}

    void bindPipeline(Pipeline& pipeline);
    void pushConstants(Pipeline& pipeline, const void* pushData);

    void traceRays(RayTracingPipeline& rtPipeline, uint32_t countX, uint32_t countY);
    void dispatch(ComputePipeline& compPipeline, uint32_t countX, uint32_t countY);

    void clearBackImage(std::array<float, 4> color);
    void clearColorImage(Image& image, std::array<float, 4> color);

    // render pass
    void beginDefaultRenderPass();
    void beginRenderPass(RenderPass& renderPass);
    void endDefaultRenderPass();
    void endRenderPass(RenderPass& renderPass);

    // submit
    void submit();

    // draw
    void drawIndexed(const Mesh& mesh);
    void drawIndexed(const DeviceBuffer& vertexBuffer,
                     const DeviceBuffer& indexBuffer,
                     uint32_t indexCount,
                     uint32_t firstIndex = 0) const;
    void drawGUI(GUI& gui);
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

    vk::CommandBuffer commandBuffer;
    Swapchain* swapchain = nullptr;
};
