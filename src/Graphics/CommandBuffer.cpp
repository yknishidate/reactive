#include "Graphics/CommandBuffer.hpp"

#include "Graphics/Buffer.hpp"
#include "Graphics/Context.hpp"
#include "Graphics/Image.hpp"
#include "Graphics/Pipeline.hpp"
#include "Timer/GPUTimer.hpp"
#include "common.hpp"

namespace rv {
auto CommandBuffer::getQueueFlags() const -> vk::QueueFlags {
    return queueFlags;
}

void CommandBuffer::begin(vk::CommandBufferUsageFlags flags) const {
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.setFlags(flags);
    commandBuffer->begin(beginInfo);
}

void CommandBuffer::end() const {
    commandBuffer->end();
}

void CommandBuffer::bindDescriptorSet(DescriptorSetHandle descSet, PipelineHandle pipeline) const {
    commandBuffer->bindDescriptorSets(pipeline->getPipelineBindPoint(),
                                      pipeline->getPipelineLayout(), 0, *descSet->descSet, nullptr);
}

void CommandBuffer::bindPipeline(PipelineHandle pipeline) const {
    commandBuffer->bindPipeline(pipeline->bindPoint, *pipeline->pipeline);
}

void CommandBuffer::pushConstants(PipelineHandle pipeline, const void* pushData) const {
    commandBuffer->pushConstants(*pipeline->pipelineLayout, pipeline->shaderStageFlags, 0,
                                 pipeline->pushSize, pushData);
}

void CommandBuffer::bindVertexBuffer(BufferHandle buffer, vk::DeviceSize offset) const {
    commandBuffer->bindVertexBuffers(0, buffer->getBuffer(), offset);
}

void CommandBuffer::bindIndexBuffer(BufferHandle buffer, vk::DeviceSize offset) const {
    commandBuffer->bindIndexBuffer(buffer->getBuffer(), 0, vk::IndexType::eUint32);
}

void CommandBuffer::traceRays(RayTracingPipelineHandle pipeline,
                              uint32_t countX,
                              uint32_t countY,
                              uint32_t countZ) const {
    commandBuffer->traceRaysKHR(pipeline->raygenRegion, pipeline->missRegion, pipeline->hitRegion,
                                {}, countX, countY, countZ);
}

void CommandBuffer::dispatch(uint32_t countX, uint32_t countY, uint32_t countZ) const {
    commandBuffer->dispatch(countX, countY, countZ);
}

void CommandBuffer::dispatchIndirect(BufferHandle buffer, vk::DeviceSize offset) const {
    commandBuffer->dispatchIndirect(buffer->getBuffer(), offset);
}

void CommandBuffer::clearColorImage(ImageHandle image, std::array<float, 4> color) const {
    transitionLayout(image, vk::ImageLayout::eTransferDstOptimal);
    commandBuffer->clearColorImage(
        image->getImage(), vk::ImageLayout::eTransferDstOptimal, vk::ClearColorValue{color},
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
}

void CommandBuffer::clearDepthStencilImage(ImageHandle image, float depth, uint32_t stencil) const {
    transitionLayout(image, vk::ImageLayout::eTransferDstOptimal);
    commandBuffer->clearDepthStencilImage(
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
    vk::RenderingAttachmentInfo depthStencilAttachment;
    if (depthImage) {
        depthStencilAttachment.setImageView(depthImage->getView());
        depthStencilAttachment.setImageLayout(vk::ImageLayout::eAttachmentOptimal);
        renderingInfo.setPDepthAttachment(&depthStencilAttachment);
    }

    commandBuffer->beginRendering(renderingInfo);
}

void CommandBuffer::endRendering() const {
    commandBuffer->endRendering();
}

void CommandBuffer::draw(uint32_t vertexCount,
                         uint32_t instanceCount,
                         uint32_t firstVertex,
                         uint32_t firstInstance) const {
    commandBuffer->draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::drawIndexed(uint32_t indexCount,
                                uint32_t instanceCount,
                                uint32_t firstIndex,
                                int32_t vertexOffset,
                                uint32_t firstInstance) const {
    commandBuffer->drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::drawIndexed(BufferHandle vertexBuffer,
                                BufferHandle indexBuffer,
                                uint32_t indexCount,
                                uint32_t firstIndex,
                                uint32_t instanceCount,
                                uint32_t firstInstance) const {
    vk::DeviceSize offsets{0};
    commandBuffer->bindVertexBuffers(0, vertexBuffer->getBuffer(), offsets);
    commandBuffer->bindIndexBuffer(indexBuffer->getBuffer(), 0, vk::IndexType::eUint32);
    commandBuffer->drawIndexed(indexCount, instanceCount, firstIndex, 0, firstInstance);
}

void CommandBuffer::drawMeshTasks(uint32_t groupCountX,
                                  uint32_t groupCountY,
                                  uint32_t groupCountZ) const {
    commandBuffer->drawMeshTasksEXT(groupCountX, groupCountY, groupCountZ);
}

void CommandBuffer::drawIndirect(BufferHandle buffer,
                                 vk::DeviceSize offset,
                                 uint32_t drawCount,
                                 uint32_t stride) const {
    commandBuffer->drawIndirect(buffer->getBuffer(), offset, drawCount, stride);
}

void CommandBuffer::drawIndexedIndirect(BufferHandle buffer,
                                        vk::DeviceSize offset,
                                        uint32_t drawCount,
                                        uint32_t stride) const {
    commandBuffer->drawIndexedIndirect(buffer->getBuffer(), offset, drawCount, stride);
}

void CommandBuffer::drawMeshTasksIndirect(BufferHandle buffer,
                                          vk::DeviceSize offset,
                                          uint32_t drawCount,
                                          uint32_t stride) const {
    commandBuffer->drawMeshTasksIndirectEXT(buffer->getBuffer(), offset, drawCount, stride);
}

void CommandBuffer::bufferBarrier(BufferHandle buffer,
                                  vk::PipelineStageFlags srcStageMask,
                                  vk::PipelineStageFlags dstStageMask,
                                  vk::AccessFlags srcAccessMask,
                                  vk::AccessFlags dstAccessMask,
                                  vk::DependencyFlags dependencyFlags) const {
    vk::BufferMemoryBarrier bufferMemoryBarrier;
    bufferMemoryBarrier.srcAccessMask = srcAccessMask;
    bufferMemoryBarrier.dstAccessMask = dstAccessMask;
    bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferMemoryBarrier.buffer = buffer->getBuffer();
    bufferMemoryBarrier.offset = 0;
    bufferMemoryBarrier.size = VK_WHOLE_SIZE;

    commandBuffer->pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, nullptr,
                                   bufferMemoryBarrier, nullptr);
}

void CommandBuffer::bufferBarrier(
    const vk::ArrayProxy<const vk::BufferMemoryBarrier>& bufferMemoryBarriers,
    vk::PipelineStageFlags srcStageMask,
    vk::PipelineStageFlags dstStageMask,
    vk::DependencyFlags dependencyFlags) const {
    commandBuffer->pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, nullptr,
                                   bufferMemoryBarriers, nullptr);
}

void CommandBuffer::imageBarrier(
    const vk::ArrayProxy<const vk::ImageMemoryBarrier>& imageMemoryBarriers,
    vk::PipelineStageFlags srcStageMask,
    vk::PipelineStageFlags dstStageMask,
    vk::DependencyFlags dependencyFlags) const {
    commandBuffer->pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, nullptr, nullptr,
                                   imageMemoryBarriers);
}

void CommandBuffer::imageBarrier(ImageHandle image,
                                 vk::PipelineStageFlags srcStageMask,
                                 vk::PipelineStageFlags dstStageMask,
                                 vk::AccessFlags srcAccessMask,
                                 vk::AccessFlags dstAccessMask,
                                 vk::DependencyFlags dependencyFlags) const {
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

    commandBuffer->pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, nullptr, nullptr,
                                   memoryBarrier);
}

void CommandBuffer::transitionLayout(ImageHandle image, vk::ImageLayout newLayout) const {
    vk::PipelineStageFlags srcStageMask = vk::PipelineStageFlagBits::eAllCommands;
    vk::PipelineStageFlags dstStageMask = vk::PipelineStageFlagBits::eAllCommands;

    vk::ImageMemoryBarrier barrier{};
    barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setImage(image->image);
    barrier.setOldLayout(image->layout);
    barrier.setNewLayout(newLayout);
    barrier.setSubresourceRange({image->aspect, 0, image->mipLevels, 0, 1});

    switch (image->layout) {
        case vk::ImageLayout::eTransferDstOptimal:
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            break;
        case vk::ImageLayout::eTransferSrcOptimal:
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
            break;
        case vk::ImageLayout::eColorAttachmentOptimal:
            barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead;
            break;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            barrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead;
            break;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
            break;
        case vk::ImageLayout::ePresentSrcKHR:
            barrier.srcAccessMask = vk::AccessFlagBits::eMemoryRead;
            break;
        default:
            break;
    }

    switch (newLayout) {
        case vk::ImageLayout::eTransferDstOptimal:
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
            break;
        case vk::ImageLayout::eTransferSrcOptimal:
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
            break;
        case vk::ImageLayout::eColorAttachmentOptimal:
            barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
            break;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            break;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            break;
        case vk::ImageLayout::ePresentSrcKHR:
            barrier.dstAccessMask = vk::AccessFlagBits::eMemoryRead;
            break;
        default:
            break;
    }

    commandBuffer->pipelineBarrier(srcStageMask, dstStageMask, {}, {}, {}, barrier);
    image->layout = newLayout;
}

void CommandBuffer::memoryBarrier(
    vk::PipelineStageFlags srcStageMask,
    vk::PipelineStageFlags dstStageMask,
    vk::DependencyFlags dependencyFlags,
    vk::ArrayProxy<const vk::MemoryBarrier> const& memoryBarriers) const {
    commandBuffer->pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, memoryBarriers,
                                   nullptr, nullptr);
}

void CommandBuffer::memoryBarrier(vk::PipelineStageFlags srcStageMask,
                                  vk::PipelineStageFlags dstStageMask,
                                  vk::AccessFlags srcAccessMask,
                                  vk::AccessFlags dstAccessMask,
                                  vk::DependencyFlags dependencyFlags) {
    vk::MemoryBarrier memoryBarrier{};
    memoryBarrier.setSrcAccessMask(srcAccessMask);
    memoryBarrier.setDstAccessMask(dstAccessMask);
    commandBuffer->pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, memoryBarrier,
                                   nullptr, nullptr);
}

void CommandBuffer::copyImage(ImageHandle srcImage,
                              ImageHandle dstImage,
                              vk::ImageLayout newSrcLayout,
                              vk::ImageLayout newDstLayout) const {
    vk::Extent3D srcExtent = srcImage->getExtent();
    vk::Extent3D dstExtent = dstImage->getExtent();
    RV_ASSERT(srcExtent == dstExtent,
              "srcImage({}, {}, {}) and dstImage({}, {}, {}) must have same extents.",
              srcExtent.width, srcExtent.height, srcExtent.depth,  // break
              dstExtent.width, dstExtent.height, dstExtent.depth)

    transitionLayout(srcImage, vk::ImageLayout::eTransferSrcOptimal);
    transitionLayout(dstImage, vk::ImageLayout::eTransferDstOptimal);

    vk::ImageCopy copyRegion;
    copyRegion.setSrcSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});
    copyRegion.setDstSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});
    copyRegion.setExtent(srcImage->getExtent());
    commandBuffer->copyImage(srcImage->getImage(), vk::ImageLayout::eTransferSrcOptimal,  // src
                             dstImage->getImage(), vk::ImageLayout::eTransferDstOptimal,  // dst
                             copyRegion);

    transitionLayout(srcImage, newSrcLayout);
    transitionLayout(dstImage, newDstLayout);
}

void CommandBuffer::copyImageToBuffer(ImageHandle srcImage, BufferHandle dstBuffer) const {
    vk::BufferImageCopy region;
    region.setImageExtent(srcImage->getExtent());
    region.setImageSubresource({srcImage->getAspectMask(), 0, 0, 1});
    commandBuffer->copyImageToBuffer(srcImage->getImage(), srcImage->getLayout(),
                                     dstBuffer->getBuffer(), region);
}

void CommandBuffer::copyBufferToImage(BufferHandle srcBuffer, ImageHandle dstImage) const {
    vk::BufferImageCopy region;
    region.setImageExtent(dstImage->getExtent());
    region.setImageSubresource({dstImage->getAspectMask(), 0, 0, 1});
    commandBuffer->copyBufferToImage(srcBuffer->getBuffer(), dstImage->getImage(),
                                     dstImage->getLayout(), region);
}

void CommandBuffer::blitImage(ImageHandle srcImage,
                              ImageHandle dstImage,
                              vk::ImageBlit blit,
                              vk::Filter filter) const {
    commandBuffer->blitImage(srcImage->image, srcImage->layout, dstImage->image, dstImage->layout,
                             blit, filter);
}

void CommandBuffer::fillBuffer(BufferHandle dstBuffer,
                               vk::DeviceSize dstOffset,
                               vk::DeviceSize size,
                               uint32_t data) const {
    commandBuffer->fillBuffer(dstBuffer->getBuffer(), dstOffset, size, data);
}

void CommandBuffer::copyBuffer(BufferHandle buffer, const void* data) const {
    buffer->prepareStagingBuffer();
    buffer->stagingBuffer->copy(data);

    vk::BufferCopy region{0, 0, buffer->getSize()};
    commandBuffer->copyBuffer(buffer->stagingBuffer->getBuffer(), buffer->getBuffer(), region);
}

void CommandBuffer::updateTopAccel(TopAccelHandle topAccel) const {
    vk::AccelerationStructureGeometryKHR geometry;
    geometry.setGeometryType(vk::GeometryTypeKHR::eInstances);
    geometry.setGeometry({topAccel->instancesData});
    geometry.setFlags(topAccel->geometryFlags);

    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
    buildGeometryInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
    buildGeometryInfo.setFlags(topAccel->buildFlags);
    buildGeometryInfo.setGeometries(geometry);

    buildGeometryInfo.setMode(vk::BuildAccelerationStructureModeKHR::eUpdate);  // for update
    buildGeometryInfo.setSrcAccelerationStructure(*topAccel->accel);            // for update
    buildGeometryInfo.setDstAccelerationStructure(*topAccel->accel);
    buildGeometryInfo.setScratchData(topAccel->scratchBuffer->getAddress());

    vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
    buildRangeInfo.setPrimitiveCount(topAccel->primitiveCount);
    buildRangeInfo.setPrimitiveOffset(0);
    buildRangeInfo.setFirstVertex(0);
    buildRangeInfo.setTransformOffset(0);
    commandBuffer->buildAccelerationStructuresKHR(buildGeometryInfo, &buildRangeInfo);
}

void CommandBuffer::buildTopAccel(TopAccelHandle topAccel) const {
    vk::AccelerationStructureGeometryKHR geometry;
    geometry.setGeometryType(vk::GeometryTypeKHR::eInstances);
    geometry.setGeometry({topAccel->instancesData});
    geometry.setFlags(topAccel->geometryFlags);

    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
    buildGeometryInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
    buildGeometryInfo.setFlags(topAccel->buildFlags);
    buildGeometryInfo.setGeometries(geometry);

    buildGeometryInfo.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);  // for build
    buildGeometryInfo.setDstAccelerationStructure(*topAccel->accel);
    buildGeometryInfo.setScratchData(topAccel->scratchBuffer->getAddress());

    vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
    buildRangeInfo.setPrimitiveCount(topAccel->primitiveCount);
    buildRangeInfo.setPrimitiveOffset(0);
    buildRangeInfo.setFirstVertex(0);
    buildRangeInfo.setTransformOffset(0);
    commandBuffer->buildAccelerationStructuresKHR(buildGeometryInfo, &buildRangeInfo);
}

void CommandBuffer::buildBottomAccel(BottomAccelHandle bottomAccel) const {
    vk::AccelerationStructureGeometryDataKHR geometryData;
    geometryData.setTriangles(bottomAccel->trianglesData);

    vk::AccelerationStructureGeometryKHR geometry;
    geometry.setGeometryType(vk::GeometryTypeKHR::eTriangles);
    geometry.setGeometry({geometryData});
    geometry.setFlags(bottomAccel->geometryFlags);

    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
    buildGeometryInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
    buildGeometryInfo.setFlags(bottomAccel->buildFlags);
    buildGeometryInfo.setGeometries(geometry);

    buildGeometryInfo.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);
    buildGeometryInfo.setDstAccelerationStructure(*bottomAccel->accel);
    buildGeometryInfo.setScratchData(bottomAccel->scratchBuffer->getAddress());

    vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
    buildRangeInfo.setPrimitiveCount(bottomAccel->primitiveCount);
    buildRangeInfo.setPrimitiveOffset(0);
    buildRangeInfo.setFirstVertex(0);
    buildRangeInfo.setTransformOffset(0);
    commandBuffer->buildAccelerationStructuresKHR(buildGeometryInfo, &buildRangeInfo);
}

void CommandBuffer::beginTimestamp(GPUTimerHandle gpuTimer) const {
    commandBuffer->resetQueryPool(*gpuTimer->queryPool, 0, 2);
    commandBuffer->writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, *gpuTimer->queryPool, 0);
}

void CommandBuffer::endTimestamp(GPUTimerHandle gpuTimer) const {
    commandBuffer->writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, *gpuTimer->queryPool,
                                  1);
}

void CommandBuffer::setLineWidth(float lineWidth) const {
    commandBuffer->setLineWidth(lineWidth);
}

void CommandBuffer::setViewport(const vk::Viewport& viewport) const {
    commandBuffer->setViewport(0, 1, &viewport);
}

void CommandBuffer::setViewport(uint32_t width, uint32_t height) const {
    vk::Viewport viewport{
        0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f,
    };
    commandBuffer->setViewport(0, 1, &viewport);
}

void CommandBuffer::setScissor(const vk::Rect2D& scissor) const {
    commandBuffer->setScissor(0, 1, &scissor);
}

void CommandBuffer::setScissor(uint32_t width, uint32_t height) const {
    vk::Rect2D scissor{
        {0, 0},
        {width, height},
    };
    commandBuffer->setScissor(0, 1, &scissor);
}

void CommandBuffer::beginDebugLabel(const char* labelName) const {
    if (context->debugEnabled()) {
        vk::DebugUtilsLabelEXT label;
        label.setPLabelName(labelName);
        commandBuffer->beginDebugUtilsLabelEXT(label);
    }
}

void CommandBuffer::endDebugLabel() const {
    if (context->debugEnabled()) {
        commandBuffer->endDebugUtilsLabelEXT();
    }
}
}  // namespace rv
