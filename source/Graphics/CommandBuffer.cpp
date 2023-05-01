#include "CommandBuffer.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include "App.hpp"
#include "Graphics/Buffer.hpp"
#include "Graphics/Image.hpp"
#include "Graphics/Pipeline.hpp"
#include "Graphics/RenderPass.hpp"
#include "Scene/Mesh.hpp"
#include "Swapchain.hpp"
#include "Timer/GPUTimer.hpp"

void CommandBuffer::bindDescriptorSet(DescriptorSet& descSet,
                                      vk::PipelineBindPoint bindPoint,
                                      vk::PipelineLayout pipelineLayout) const {
    descSet.bind(commandBuffer, bindPoint, pipelineLayout);
}

void CommandBuffer::bindPipeline(Pipeline& pipeline) const {
    pipeline.bind(commandBuffer);
}

void CommandBuffer::pushConstants(Pipeline& pipeline, const void* pushData) const {
    pipeline.pushConstants(commandBuffer, pushData);
}

// void CommandBuffer::traceRays(RayTracingPipeline& rtPipeline, uint32_t countX, uint32_t countY) {
//     rtPipeline.traceRays(commandBuffer, countX, countY);
// }
//
// void CommandBuffer::dispatch(ComputePipeline& compPipeline, uint32_t countX, uint32_t countY) {
//     compPipeline.dispatch(commandBuffer, countX, countY);
// }

// void CommandBuffer::clearColorImage(Image& image, std::array<float, 4> color) {
//     image.clearColor(commandBuffer, color);
// }

void CommandBuffer::clearColorImage(vk::Image image, std::array<float, 4> color) const {
    Image::setImageLayout(commandBuffer, image, vk::ImageLayout::eTransferDstOptimal);
    commandBuffer.clearColorImage(
        image, vk::ImageLayout::eTransferDstOptimal, vk::ClearColorValue{color},
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
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

// void CommandBuffer::beginRenderPass(RenderPass& renderPass) {
//     renderPass.beginRenderPass(commandBuffer);
// }
//
// void CommandBuffer::endRenderPass(RenderPass& renderPass) {
//    renderPass.endRenderPass(commandBuffer);
//}
//
// void CommandBuffer::submit() {
//    swapchain->submit();
//}

void CommandBuffer::drawIndexed(const Mesh& mesh) const {
    mesh.drawIndexed(commandBuffer);
}

void CommandBuffer::drawIndexed(const DeviceBuffer& vertexBuffer,
                                const DeviceBuffer& indexBuffer,
                                uint32_t indexCount,
                                uint32_t firstIndex,
                                uint32_t instanceCount,
                                uint32_t firstInstance) const {
    vk::DeviceSize offsets{0};
    commandBuffer.bindVertexBuffers(0, vertexBuffer.getBuffer(), offsets);
    commandBuffer.bindIndexBuffer(indexBuffer.getBuffer(), 0, vk::IndexType::eUint32);
    commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, 0, firstInstance);
}

void CommandBuffer::drawMeshTasks(uint32_t groupCountX,
                                  uint32_t groupCountY,
                                  uint32_t groupCountZ) const {
    commandBuffer.drawMeshTasksEXT(groupCountX, groupCountY, groupCountZ);
}

void CommandBuffer::pipelineBarrier(vk::PipelineStageFlags srcStageMask,
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

void CommandBuffer::beginTimestamp(const GPUTimer& gpuTimer) const {
    gpuTimer.beginTimestamp(commandBuffer);
}

void CommandBuffer::endTimestamp(const GPUTimer& gpuTimer) const {
    gpuTimer.endTimestamp(commandBuffer);
}
