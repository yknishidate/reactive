#include "Graphics/CommandBuffer.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>

#include "Graphics/Buffer.hpp"
#include "Graphics/Context.hpp"
#include "Graphics/Image.hpp"
#include "Graphics/Pipeline.hpp"
#include "Graphics/RenderPass.hpp"
#include "Scene/Mesh.hpp"
#include "Timer/GPUTimer.hpp"

namespace rv {
void CommandBuffer::bindDescriptorSet(DescriptorSet& descSet, const Pipeline& pipeline) const {
    descSet.bind(commandBuffer, pipeline.getPipelineBindPoint(), pipeline.getPipelineLayout());
}

void CommandBuffer::bindPipeline(Pipeline& pipeline) const {
    pipeline.bind(commandBuffer);
}

void CommandBuffer::pushConstants(Pipeline& pipeline, const void* pushData) const {
    pipeline.pushConstants(commandBuffer, pushData);
}

void CommandBuffer::bindVertexBuffer(const Buffer& buffer, vk::DeviceSize offset) const {
    commandBuffer.bindVertexBuffers(0, buffer.getBuffer(), offset);
}

void CommandBuffer::bindIndexBuffer(const Buffer& buffer, vk::DeviceSize offset) const {
    commandBuffer.bindIndexBuffer(buffer.getBuffer(), 0, vk::IndexType::eUint32);
}

void CommandBuffer::traceRays(const RayTracingPipeline& pipeline,
                              uint32_t countX,
                              uint32_t countY,
                              uint32_t countZ) const {
    pipeline.traceRays(commandBuffer, countX, countY, countZ);
}

void CommandBuffer::dispatch(const ComputePipeline& pipeline,
                             uint32_t countX,
                             uint32_t countY,
                             uint32_t countZ) const {
    pipeline.dispatch(commandBuffer, countX, countY, countZ);
}

void CommandBuffer::dispatchIndirect(const Buffer& buffer, vk::DeviceSize offset) const {
    commandBuffer.dispatchIndirect(buffer.getBuffer(), offset);
}

// void CommandBuffer::dispatch(ComputePipeline& compPipeline, uint32_t countX, uint32_t countY) {
//     compPipeline.dispatch(commandBuffer, countX, countY);
// }

// void CommandBuffer::clearColorImage(Image& image, std::array<float, 4> color) {
//     image.clearColor(commandBuffer, color);
// }

void CommandBuffer::clearColorImage(vk::Image image, std::array<float, 4> color) const {
    // TODO: Fix vk::ImageLayout::eUndefined
    Image::setImageLayout(commandBuffer, image, vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eTransferDstOptimal, vk::ImageAspectFlagBits::eColor, 1);
    commandBuffer.clearColorImage(
        image, vk::ImageLayout::eTransferDstOptimal, vk::ClearColorValue{color},
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
}

void CommandBuffer::clearDepthStencilImage(vk::Image image, float depth, uint32_t stencil) const {
    // TODO: Fix vk::ImageLayout::eUndefined
    Image::setImageLayout(commandBuffer, image, vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eTransferDstOptimal, vk::ImageAspectFlagBits::eDepth, 1);
    commandBuffer.clearDepthStencilImage(
        image, vk::ImageLayout::eTransferDstOptimal, vk::ClearDepthStencilValue{depth, stencil},
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1});
}

void CommandBuffer::beginRenderPass(vk::RenderPass renderPass,
                                    vk::Framebuffer framebuffer,
                                    uint32_t width,
                                    uint32_t height) const {
    vk::Rect2D renderArea;
    renderArea.setExtent({width, height});

    std::array<vk::ClearValue, 2> clearValues;
    clearValues[0].color = {std::array{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};

    vk::RenderPassBeginInfo beginInfo;
    beginInfo.setRenderPass(renderPass);
    beginInfo.setClearValues(clearValues);
    beginInfo.setFramebuffer(framebuffer);
    beginInfo.setRenderArea(renderArea);
    commandBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);
}

void CommandBuffer::draw(uint32_t vertexCount,
                         uint32_t instanceCount,
                         uint32_t firstVertex,
                         uint32_t firstInstance) const {
    commandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::drawIndexed(uint32_t indexCount,
                                uint32_t instanceCount,
                                uint32_t firstIndex,
                                int32_t vertexOffset,
                                uint32_t firstInstance) const {
    commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::drawIndexed(const Buffer& vertexBuffer,
                                const Buffer& indexBuffer,
                                uint32_t indexCount,
                                uint32_t firstIndex,
                                uint32_t instanceCount,
                                uint32_t firstInstance) const {
    vk::DeviceSize offsets{0};
    commandBuffer.bindVertexBuffers(0, vertexBuffer.getBuffer(), offsets);
    commandBuffer.bindIndexBuffer(indexBuffer.getBuffer(), 0, vk::IndexType::eUint32);
    commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, 0, firstInstance);
}

void CommandBuffer::drawIndexed(const Mesh& mesh, uint32_t instanceCount) const {
    mesh.drawIndexed(commandBuffer, instanceCount);
}

void CommandBuffer::drawMeshTasks(uint32_t groupCountX,
                                  uint32_t groupCountY,
                                  uint32_t groupCountZ) const {
    commandBuffer.drawMeshTasksEXT(groupCountX, groupCountY, groupCountZ);
}

void CommandBuffer::drawIndirect(const Buffer& buffer,
                                 vk::DeviceSize offset,
                                 uint32_t drawCount,
                                 uint32_t stride) const {
    commandBuffer.drawIndirect(buffer.getBuffer(), offset, drawCount, stride);
}

void CommandBuffer::drawIndexedIndirect(const Buffer& buffer,
                                        vk::DeviceSize offset,
                                        uint32_t drawCount,
                                        uint32_t stride) const {
    commandBuffer.drawIndexedIndirect(buffer.getBuffer(), offset, drawCount, stride);
}

void CommandBuffer::drawMeshTasksIndirect(const Buffer& buffer,
                                          vk::DeviceSize offset,
                                          uint32_t drawCount,
                                          uint32_t stride) const {
    commandBuffer.drawMeshTasksIndirectEXT(buffer.getBuffer(), offset, drawCount, stride);
}

void CommandBuffer::bufferBarrier(vk::PipelineStageFlags srcStageMask,
                                  vk::PipelineStageFlags dstStageMask,
                                  vk::DependencyFlags dependencyFlags,
                                  const Buffer& buffer,
                                  vk::AccessFlags srcAccessMask,
                                  vk::AccessFlags dstAccessMask) const {
    vk::BufferMemoryBarrier bufferMemoryBarrier;
    bufferMemoryBarrier.srcAccessMask = srcAccessMask;
    bufferMemoryBarrier.dstAccessMask = dstAccessMask;
    bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferMemoryBarrier.buffer = buffer.getBuffer();
    bufferMemoryBarrier.offset = 0;
    bufferMemoryBarrier.size = VK_WHOLE_SIZE;

    commandBuffer.pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, nullptr,
                                  bufferMemoryBarrier, nullptr);
}

void CommandBuffer::imageBarrier(vk::PipelineStageFlags srcStageMask,
                                 vk::PipelineStageFlags dstStageMask,
                                 vk::DependencyFlags dependencyFlags,
                                 const Image& image,
                                 vk::AccessFlags srcAccessMask,
                                 vk::AccessFlags dstAccessMask) const {
    vk::ImageMemoryBarrier memoryBarrier;
    memoryBarrier.srcAccessMask = srcAccessMask;
    memoryBarrier.dstAccessMask = dstAccessMask;
    memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    memoryBarrier.image = image.getImage();
    memoryBarrier.oldLayout = vk::ImageLayout::eGeneral;
    memoryBarrier.newLayout = vk::ImageLayout::eGeneral;
    memoryBarrier.subresourceRange.aspectMask = image.getAspectMask();
    memoryBarrier.subresourceRange.baseMipLevel = 0;
    memoryBarrier.subresourceRange.baseArrayLayer = 0;
    memoryBarrier.subresourceRange.layerCount = 1;
    memoryBarrier.subresourceRange.levelCount = 1;

    commandBuffer.pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, nullptr, nullptr,
                                  memoryBarrier);
}

void CommandBuffer::copyImage(vk::Image srcImage,
                              vk::Image dstImage,
                              vk::ImageLayout newSrcLayout,
                              vk::ImageLayout newDstLayout,
                              uint32_t width,
                              uint32_t height) const {
    // TODO: Fix vk::ImageLayout::eUndefined
    Image::setImageLayout(commandBuffer, srcImage, vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eTransferSrcOptimal, vk::ImageAspectFlagBits::eColor, 1);
    Image::setImageLayout(commandBuffer, dstImage, vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eTransferDstOptimal, vk::ImageAspectFlagBits::eColor, 1);

    vk::ImageCopy copyRegion;
    copyRegion.setSrcSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});
    copyRegion.setDstSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});
    copyRegion.setExtent({width, height, 1});
    commandBuffer.copyImage(srcImage, vk::ImageLayout::eTransferSrcOptimal,  // src
                            dstImage, vk::ImageLayout::eTransferDstOptimal,  // dst
                            copyRegion);

    Image::setImageLayout(commandBuffer, srcImage, vk::ImageLayout::eTransferSrcOptimal,
                          newSrcLayout, vk::ImageAspectFlagBits::eColor, 1);
    Image::setImageLayout(commandBuffer, dstImage, vk::ImageLayout::eTransferDstOptimal,
                          newDstLayout, vk::ImageAspectFlagBits::eColor, 1);
}

void CommandBuffer::fillBuffer(const Buffer& dstBuffer,
                               vk::DeviceSize dstOffset,
                               vk::DeviceSize size,
                               uint32_t data) const {
    commandBuffer.fillBuffer(dstBuffer.getBuffer(), dstOffset, size, data);
}

void CommandBuffer::beginTimestamp(const GPUTimer& gpuTimer) const {
    gpuTimer.beginTimestamp(commandBuffer);
}

void CommandBuffer::endTimestamp(const GPUTimer& gpuTimer) const {
    gpuTimer.endTimestamp(commandBuffer);
}
}  // namespace rv
