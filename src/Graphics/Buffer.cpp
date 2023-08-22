#include "Graphics/Buffer.hpp"

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

vk::DeviceAddress Buffer::getAddress() const {
    vk::BufferDeviceAddressInfo bufferDeviceAI{*buffer};
    return context->getDevice().getBufferAddress(&bufferDeviceAI);
}

void* Buffer::map() {
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
        context->oneTimeSubmit([&](vk::CommandBuffer commandBuffer) {
            vk::BufferCopy region{0, 0, size};
            commandBuffer.copyBuffer(stagingBuffer->getBuffer(), *buffer, region);
        });
    }
}
}  // namespace rv
