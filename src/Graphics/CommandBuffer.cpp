#include "Graphics/CommandBuffer.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>

#include "Graphics/Buffer.hpp"
#include "Graphics/Context.hpp"
#include "Graphics/Image.hpp"
#include "Graphics/Pipeline.hpp"
#include "Scene/Mesh.hpp"
#include "Timer/GPUTimer.hpp"

namespace rv {
void CommandBuffer::bindDescriptorSet(DescriptorSetHandle descSet, PipelineHandle pipeline) const {
    descSet->bind(commandBuffer, pipeline->getPipelineBindPoint(), pipeline->getPipelineLayout());
}

void CommandBuffer::bindPipeline(PipelineHandle pipeline) const {
    pipeline->bind(commandBuffer);
}

void CommandBuffer::pushConstants(PipelineHandle pipeline, const void* pushData) const {
    pipeline->pushConstants(commandBuffer, pushData);
}

void CommandBuffer::bindVertexBuffer(BufferHandle buffer, vk::DeviceSize offset) const {
    commandBuffer.bindVertexBuffers(0, buffer->getBuffer(), offset);
}

void CommandBuffer::bindIndexBuffer(BufferHandle buffer, vk::DeviceSize offset) const {
    commandBuffer.bindIndexBuffer(buffer->getBuffer(), 0, vk::IndexType::eUint32);
}

void CommandBuffer::traceRays(RayTracingPipelineHandle pipeline,
                              uint32_t countX,
                              uint32_t countY,
                              uint32_t countZ) const {
    pipeline->traceRays(commandBuffer, countX, countY, countZ);
}

void CommandBuffer::dispatch(ComputePipelineHandle pipeline,
                             uint32_t countX,
                             uint32_t countY,
                             uint32_t countZ) const {
    pipeline->dispatch(commandBuffer, countX, countY, countZ);
}

void CommandBuffer::dispatchIndirect(BufferHandle buffer, vk::DeviceSize offset) const {
    commandBuffer.dispatchIndirect(buffer->getBuffer(), offset);
}

// void CommandBuffer::dispatch(ComputePipeline& compPipeline, uint32_t countX, uint32_t countY) {
//     compPipeline.dispatch(commandBuffer, countX, countY);
// }

// void CommandBuffer::clearColorImage(Image& image, std::array<float, 4> color) {
//     image.clearColor(commandBuffer, color);
// }

void CommandBuffer::clearColorImage(ImageHandle image, std::array<float, 4> color) const {
    // TODO: Fix vk::ImageLayout::eUndefined
    Image::transitionLayout(commandBuffer, image->getImage(), vk::ImageLayout::eUndefined,
                            vk::ImageLayout::eTransferDstOptimal, vk::ImageAspectFlagBits::eColor,
                            1);
    commandBuffer.clearColorImage(
        image->getImage(), vk::ImageLayout::eTransferDstOptimal, vk::ClearColorValue{color},
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
}

void CommandBuffer::clearDepthStencilImage(ImageHandle image, float depth, uint32_t stencil) const {
    // TODO: Fix vk::ImageLayout::eUndefined
    Image::transitionLayout(commandBuffer, image->getImage(), vk::ImageLayout::eUndefined,
                            vk::ImageLayout::eTransferDstOptimal, vk::ImageAspectFlagBits::eDepth,
                            1);
    commandBuffer.clearDepthStencilImage(
        image->getImage(), vk::ImageLayout::eTransferDstOptimal,
        vk::ClearDepthStencilValue{depth, stencil},
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1});
}

void CommandBuffer::beginRendering(ImageHandle colorImage,
                                   ImageHandle depthImage,
                                   std::array<int32_t, 2> offset,
                                   std::array<uint32_t, 2> extent) const {
    vk::RenderingInfo renderingInfo;
    renderingInfo.setRenderArea({{offset[0], offset[1]}, {extent[0], extent[1]}});
    renderingInfo.setLayerCount(1);

    // NOTE: Attachments support only explicit clear commands.
    // Therefore, clearing is not performed within beginRendering.
    vk::RenderingAttachmentInfo colorAttachment;
    colorAttachment.setImageView(colorImage->getView());
    colorAttachment.setImageLayout(vk::ImageLayout::eAttachmentOptimal);
    renderingInfo.setColorAttachments(colorAttachment);

    // Depth attachment
    if (depthImage) {
        vk::RenderingAttachmentInfo depthStencilAttachment;
        depthStencilAttachment.setImageView(depthImage->getView());
        depthStencilAttachment.setImageLayout(vk::ImageLayout::eAttachmentOptimal);
        renderingInfo.setPDepthAttachment(&depthStencilAttachment);
    }

    commandBuffer.beginRendering(renderingInfo);
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

void CommandBuffer::drawIndexed(BufferHandle vertexBuffer,
                                BufferHandle indexBuffer,
                                uint32_t indexCount,
                                uint32_t firstIndex,
                                uint32_t instanceCount,
                                uint32_t firstInstance) const {
    vk::DeviceSize offsets{0};
    commandBuffer.bindVertexBuffers(0, vertexBuffer->getBuffer(), offsets);
    commandBuffer.bindIndexBuffer(indexBuffer->getBuffer(), 0, vk::IndexType::eUint32);
    commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, 0, firstInstance);
}

void CommandBuffer::drawIndexed(MeshHandle mesh, uint32_t instanceCount) const {
    mesh->drawIndexed(commandBuffer, instanceCount);
}

void CommandBuffer::drawMeshTasks(uint32_t groupCountX,
                                  uint32_t groupCountY,
                                  uint32_t groupCountZ) const {
    commandBuffer.drawMeshTasksEXT(groupCountX, groupCountY, groupCountZ);
}

void CommandBuffer::drawIndirect(BufferHandle buffer,
                                 vk::DeviceSize offset,
                                 uint32_t drawCount,
                                 uint32_t stride) const {
    commandBuffer.drawIndirect(buffer->getBuffer(), offset, drawCount, stride);
}

void CommandBuffer::drawIndexedIndirect(BufferHandle buffer,
                                        vk::DeviceSize offset,
                                        uint32_t drawCount,
                                        uint32_t stride) const {
    commandBuffer.drawIndexedIndirect(buffer->getBuffer(), offset, drawCount, stride);
}

void CommandBuffer::drawMeshTasksIndirect(BufferHandle buffer,
                                          vk::DeviceSize offset,
                                          uint32_t drawCount,
                                          uint32_t stride) const {
    commandBuffer.drawMeshTasksIndirectEXT(buffer->getBuffer(), offset, drawCount, stride);
}

void CommandBuffer::bufferBarrier(vk::PipelineStageFlags srcStageMask,
                                  vk::PipelineStageFlags dstStageMask,
                                  vk::DependencyFlags dependencyFlags,
                                  BufferHandle buffer,
                                  vk::AccessFlags srcAccessMask,
                                  vk::AccessFlags dstAccessMask) const {
    vk::BufferMemoryBarrier bufferMemoryBarrier;
    bufferMemoryBarrier.srcAccessMask = srcAccessMask;
    bufferMemoryBarrier.dstAccessMask = dstAccessMask;
    bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferMemoryBarrier.buffer = buffer->getBuffer();
    bufferMemoryBarrier.offset = 0;
    bufferMemoryBarrier.size = VK_WHOLE_SIZE;

    commandBuffer.pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, nullptr,
                                  bufferMemoryBarrier, nullptr);
}

void CommandBuffer::imageBarrier(vk::PipelineStageFlags srcStageMask,
                                 vk::PipelineStageFlags dstStageMask,
                                 vk::DependencyFlags dependencyFlags,
                                 ImageHandle image,
                                 vk::AccessFlags srcAccessMask,
                                 vk::AccessFlags dstAccessMask) const {
    // NOTE: Since layout transition is not required,
    // oldLayout and newLayout are not specified.
    vk::ImageMemoryBarrier memoryBarrier;
    memoryBarrier.srcAccessMask = srcAccessMask;
    memoryBarrier.dstAccessMask = dstAccessMask;
    memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    memoryBarrier.image = image->getImage();
    memoryBarrier.subresourceRange.aspectMask = image->getAspectMask();
    memoryBarrier.subresourceRange.baseMipLevel = 0;
    memoryBarrier.subresourceRange.baseArrayLayer = 0;
    memoryBarrier.subresourceRange.layerCount = 1;
    memoryBarrier.subresourceRange.levelCount = 1;

    commandBuffer.pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, nullptr, nullptr,
                                  memoryBarrier);
}

void CommandBuffer::transitionLayout(vk::Image image,
                                     vk::ImageLayout oldLayout,
                                     vk::ImageLayout newLayout,
                                     vk::ImageAspectFlagBits aspect,
                                     uint32_t mipLevels) const {
    Image::transitionLayout(commandBuffer, image, oldLayout, newLayout, aspect, mipLevels);
}

void CommandBuffer::copyImage(ImageHandle srcImage,
                              ImageHandle dstImage,
                              ImageLayout newSrcLayout,
                              ImageLayout newDstLayout,
                              uint32_t width,
                              uint32_t height) const {
    // TODO: Fix vk::ImageLayout::eUndefined
    Image::transitionLayout(commandBuffer, srcImage->getImage(), vk::ImageLayout::eUndefined,
                            vk::ImageLayout::eTransferSrcOptimal, vk::ImageAspectFlagBits::eColor,
                            1);
    Image::transitionLayout(commandBuffer, dstImage->getImage(), vk::ImageLayout::eUndefined,
                            vk::ImageLayout::eTransferDstOptimal, vk::ImageAspectFlagBits::eColor,
                            1);

    vk::ImageCopy copyRegion;
    copyRegion.setSrcSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});
    copyRegion.setDstSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});
    copyRegion.setExtent({width, height, 1});
    commandBuffer.copyImage(srcImage->getImage(), vk::ImageLayout::eTransferSrcOptimal,  // src
                            dstImage->getImage(), vk::ImageLayout::eTransferDstOptimal,  // dst
                            copyRegion);

    Image::transitionLayout(commandBuffer, srcImage->getImage(),
                            vk::ImageLayout::eTransferSrcOptimal, getImageLayout(newSrcLayout),
                            vk::ImageAspectFlagBits::eColor, 1);
    Image::transitionLayout(commandBuffer, dstImage->getImage(),
                            vk::ImageLayout::eTransferDstOptimal, getImageLayout(newDstLayout),
                            vk::ImageAspectFlagBits::eColor, 1);
}

void CommandBuffer::fillBuffer(BufferHandle dstBuffer,
                               vk::DeviceSize dstOffset,
                               vk::DeviceSize size,
                               uint32_t data) const {
    commandBuffer.fillBuffer(dstBuffer->getBuffer(), dstOffset, size, data);
}

void CommandBuffer::beginTimestamp(GPUTimerHandle gpuTimer) const {
    gpuTimer->beginTimestamp(commandBuffer);
}

void CommandBuffer::endTimestamp(GPUTimerHandle gpuTimer) const {
    gpuTimer->endTimestamp(commandBuffer);
}
}  // namespace rv
