#include "Graphics/Buffer.hpp"

Buffer::Buffer(const Context* context,
               BufferUsage usage,
               vk::MemoryPropertyFlags memoryProp,
               size_t size)
    : context{context}, size(size) {
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
        case BufferUsage::Uniform:
            usageFlag = vk::BufferUsageFlagBits::eUniformBuffer |
                        vk::BufferUsageFlagBits::eTransferDst |
                        vk::BufferUsageFlagBits::eShaderDeviceAddress;
            break;
        case BufferUsage::Indirect:
            usageFlag = vk::BufferUsageFlagBits::eStorageBuffer |
                        vk::BufferUsageFlagBits::eTransferDst |
                        vk::BufferUsageFlagBits::eIndirectBuffer;
            break;
        default:
            assert(false);
    }
    buffer = context->getDevice().createBufferUnique({{}, size, usageFlag});

    vk::MemoryRequirements requirements = context->getDevice().getBufferMemoryRequirements(*buffer);
    uint32_t memoryTypeIndex = context->findMemoryTypeIndex(requirements, memoryProp);
    vk::MemoryAllocateFlagsInfo flagsInfo{vk::MemoryAllocateFlagBits::eDeviceAddress};
    memory = context->getDevice().allocateMemoryUnique(vk::MemoryAllocateInfo()
                                                           .setAllocationSize(requirements.size)
                                                           .setMemoryTypeIndex(memoryTypeIndex)
                                                           .setPNext(&flagsInfo));

    context->getDevice().bindBufferMemory(*buffer, *memory, 0);
}

HostBuffer::HostBuffer(const Context* context, BufferCreateInfo createInfo)
    : Buffer(context,
             createInfo.usage,
             vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
             createInfo.size) {
    if (createInfo.data) {
        copy(createInfo.data);
    }
}

void HostBuffer::copy(const void* data) {
    map();
    std::memcpy(mapped, data, size);
}

void* HostBuffer::map() {
    if (!mapped) {
        mapped = context->getDevice().mapMemory(*memory, 0, VK_WHOLE_SIZE);
    }
    return mapped;
}

void HostBuffer::unmap() {
    context->getDevice().unmapMemory(*memory);
    mapped = nullptr;
}

DeviceBuffer::DeviceBuffer(const Context* context, BufferCreateInfo createInfo)
    : Buffer(context, createInfo.usage, vk::MemoryPropertyFlagBits::eDeviceLocal, createInfo.size) {
    if (createInfo.data) {
        copy(createInfo.data);
    }
}

void DeviceBuffer::copy(const void* data) {
    HostBuffer stagingBuffer = context->createHostBuffer({
        .usage = BufferUsage::Staging,
        .size = size,
        .data = data,
    });
    context->oneTimeSubmit([&](vk::CommandBuffer commandBuffer) {
        vk::BufferCopy region{0, 0, size};
        commandBuffer.copyBuffer(stagingBuffer.getBuffer(), *buffer, region);
    });
}
