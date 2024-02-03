#include "Graphics/Buffer.hpp"

#include "Graphics/CommandBuffer.hpp"
#include "common.hpp"

namespace rv {
Buffer::Buffer(const Context& _context, const BufferCreateInfo& createInfo)
    : context{&_context}, size{createInfo.size} {
    // Create buffer
    vk::BufferCreateInfo bufferInfo;
    bufferInfo.setSize(size);
    bufferInfo.setUsage(createInfo.usage);
    buffer = context->getDevice().createBufferUnique(bufferInfo);

    // Allocate memory
    vk::MemoryRequirements requirements = context->getDevice().getBufferMemoryRequirements(*buffer);
    uint32_t memoryTypeIndex = context->findMemoryTypeIndex(requirements, createInfo.memory);

    vk::MemoryAllocateFlagsInfo flagsInfo{vk::MemoryAllocateFlagBits::eDeviceAddress};
    vk::MemoryAllocateInfo memoryInfo;
    memoryInfo.setAllocationSize(requirements.size);
    memoryInfo.setMemoryTypeIndex(memoryTypeIndex);
    memoryInfo.setPNext(&flagsInfo);
    memory = context->getDevice().allocateMemoryUnique(memoryInfo);

    isHostVisible = static_cast<bool>(createInfo.memory & vk::MemoryPropertyFlagBits::eHostVisible);

    // Bind memory
    context->getDevice().bindBufferMemory(*buffer, *memory, 0);

    if (!createInfo.debugName.empty()) {
        context->setDebugName(*buffer, createInfo.debugName.c_str());
        context->setDebugName(*memory, createInfo.debugName.c_str());
    }
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
    RV_ASSERT(isHostVisible, "This buffer is not host visible.");
    context->getDevice().unmapMemory(*memory);
    mapped = nullptr;
}

void Buffer::copy(const void* data) {
    RV_ASSERT(isHostVisible, "This buffer is not host visible.");
    map();
    std::memcpy(mapped, data, size);
}

void Buffer::prepareStagingBuffer() {
    RV_ASSERT(!isHostVisible, "This buffer is not device buffer.");
    if (!stagingBuffer) {
        stagingBuffer = context->createBuffer({
            .usage = BufferUsage::Staging,
            .memory = MemoryUsage::Host,
            .size = size,
        });
    }
}
}  // namespace rv
