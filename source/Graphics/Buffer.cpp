#include "Buffer.hpp"

Buffer::Buffer(BufferUsage usage, vk::MemoryPropertyFlags memoryProp, size_t size) : size(size) {
    vk::BufferUsageFlags usageFlag;
    switch (usage) {
        case BufferUsage::Vertex:
            usageFlag = vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                        vk::BufferUsageFlagBits::eStorageBuffer |
                        vk::BufferUsageFlagBits::eShaderDeviceAddress |
                        vk::BufferUsageFlagBits::eVertexBuffer |
                        vk::BufferUsageFlagBits::eTransferDst;
            break;
        case BufferUsage::Index:
            usageFlag = vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                        vk::BufferUsageFlagBits::eStorageBuffer |
                        vk::BufferUsageFlagBits::eShaderDeviceAddress |
                        vk::BufferUsageFlagBits::eIndexBuffer |
                        vk::BufferUsageFlagBits::eTransferDst;
            break;
        case BufferUsage::AccelInput:
            usageFlag = vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                        vk::BufferUsageFlagBits::eStorageBuffer |
                        vk::BufferUsageFlagBits::eShaderDeviceAddress |
                        vk::BufferUsageFlagBits::eTransferDst;
            break;
        case BufferUsage::AccelStorage:
            usageFlag = vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
                        vk::BufferUsageFlagBits::eShaderDeviceAddress;
            break;
        case BufferUsage::Scratch:
            usageFlag = vk::BufferUsageFlagBits::eStorageBuffer |
                        vk::BufferUsageFlagBits::eShaderDeviceAddress;
            break;
        case BufferUsage::Staging:
            usageFlag =
                vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;
            break;
        case BufferUsage::Storage:
            usageFlag = vk::BufferUsageFlagBits::eStorageBuffer |
                        vk::BufferUsageFlagBits::eTransferDst |
                        vk::BufferUsageFlagBits::eShaderDeviceAddress;
            break;
        case BufferUsage::ShaderBindingTable:
            usageFlag = vk::BufferUsageFlagBits::eShaderBindingTableKHR |
                        vk::BufferUsageFlagBits::eShaderDeviceAddress |
                        vk::BufferUsageFlagBits::eTransferDst;
            break;
        default:
            assert(false);
    }
    buffer = Context::getDevice().createBufferUnique({{}, size, usageFlag});

    vk::MemoryRequirements requirements = Context::getDevice().getBufferMemoryRequirements(*buffer);
    uint32_t memoryTypeIndex = Context::findMemoryTypeIndex(requirements, memoryProp);
    vk::MemoryAllocateFlagsInfo flagsInfo{vk::MemoryAllocateFlagBits::eDeviceAddress};
    memory = Context::getDevice().allocateMemoryUnique(vk::MemoryAllocateInfo()
                                                           .setAllocationSize(requirements.size)
                                                           .setMemoryTypeIndex(memoryTypeIndex)
                                                           .setPNext(&flagsInfo));

    Context::getDevice().bindBufferMemory(*buffer, *memory, 0);
}

HostBuffer::HostBuffer(BufferUsage usage, size_t size)
    : Buffer(usage,
             vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
             size) {}

void HostBuffer::copy(const void* data) {
    map();
    std::memcpy(mapped, data, size);
}

void* HostBuffer::map() {
    if (!mapped) {
        mapped = Context::getDevice().mapMemory(*memory, 0, VK_WHOLE_SIZE);
    }
    return mapped;
}

DeviceBuffer::DeviceBuffer(BufferUsage usage, size_t size)
    : Buffer(usage, vk::MemoryPropertyFlagBits::eDeviceLocal, size) {}

void DeviceBuffer::copy(const void* data) {
    HostBuffer stagingBuffer{BufferUsage::Staging, size};
    stagingBuffer.copy(data);
    Context::oneTimeSubmit([&](vk::CommandBuffer commandBuffer) {
        vk::BufferCopy region{0, 0, size};
        commandBuffer.copyBuffer(stagingBuffer.getBuffer(), *buffer, region);
    });
}
