#include "Graphics/Buffer.hpp"

#include "Graphics/CommandBuffer.hpp"
#include "common.hpp"

namespace rv {
Buffer::Buffer(const Context* context,
               vk::BufferUsageFlags usage,
               vk::MemoryPropertyFlags memoryProp,
               vk::DeviceSize size,
               const void* data,
               const char* debugName)
    : context{context}, size(size) {
    // Create buffer
    vk::BufferCreateInfo bufferInfo;
    bufferInfo.setSize(size);
    bufferInfo.setUsage(usage);
    buffer = context->getDevice().createBufferUnique(bufferInfo);

    // Allocate memory
    vk::MemoryRequirements requirements = context->getDevice().getBufferMemoryRequirements(*buffer);
    uint32_t memoryTypeIndex = context->findMemoryTypeIndex(requirements, memoryProp);

    vk::MemoryAllocateFlagsInfo flagsInfo{vk::MemoryAllocateFlagBits::eDeviceAddress};
    vk::MemoryAllocateInfo memoryInfo;
    memoryInfo.setAllocationSize(requirements.size);
    memoryInfo.setMemoryTypeIndex(memoryTypeIndex);
    memoryInfo.setPNext(&flagsInfo);
    memory = context->getDevice().allocateMemoryUnique(memoryInfo);

    isHostVisible = static_cast<bool>(memoryProp & vk::MemoryPropertyFlagBits::eHostVisible);

    // Bind memory
    context->getDevice().bindBufferMemory(*buffer, *memory, 0);

    // Copy data
    if (data) {
        copy(data);
    }

    if (debugName) {
        context->setDebugName(*buffer, debugName);
        context->setDebugName(*memory, debugName);
    }
}

auto Buffer::getBuffer() const -> vk::Buffer {
    return *buffer;
}

auto Buffer::getSize() const -> vk::DeviceSize {
    return size;
}

auto Buffer::getInfo() const -> vk::DescriptorBufferInfo {
    return {*buffer, 0, size};
}

auto Buffer::getAddress() const -> vk::DeviceAddress {
    vk::BufferDeviceAddressInfo addressInfo{*buffer};
    return context->getDevice().getBufferAddress(&addressInfo);
}

auto Buffer::map() -> void* {
    RV_ASSERT(isHostVisible, "");
    if (!mapped) {
        mapped = context->getDevice().mapMemory(*memory, 0, VK_WHOLE_SIZE);
    }
    return mapped;
}

void Buffer::unmap() {
    RV_ASSERT(isHostVisible, "");
    context->getDevice().unmapMemory(*memory);
    mapped = nullptr;
}

// TODO: move to CommandBuffer
void Buffer::copy(const void* data) {
    if (isHostVisible) {
        map();
        std::memcpy(mapped, data, size);
    } else {
        BufferHandle stagingBuffer = context->createBuffer({
            .usage = BufferUsage::Staging,
            .memory = MemoryUsage::Host,
            .size = size,
            .data = data,
        });
        context->oneTimeSubmit([&](CommandBufferHandle commandBuffer) {
            vk::BufferCopy region{0, 0, size};
            commandBuffer->commandBuffer->copyBuffer(stagingBuffer->getBuffer(), *buffer, region);
        });
    }
}
}  // namespace rv
