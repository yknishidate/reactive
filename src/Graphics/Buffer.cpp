#include "Graphics/Buffer.hpp"

#include "common.hpp"

namespace {
vk::BufferUsageFlags getBufferUsage(rv::BufferUsage usage) {
    switch (usage) {
        case rv::BufferUsage::Uniform:
            return vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst |
                   vk::BufferUsageFlagBits::eShaderDeviceAddress;
        case rv::BufferUsage::Storage:
            return vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst |
                   vk::BufferUsageFlagBits::eShaderDeviceAddress;
        case rv::BufferUsage::Staging:
            return vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;
        case rv::BufferUsage::Vertex:
            return vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                   vk::BufferUsageFlagBits::eStorageBuffer |
                   vk::BufferUsageFlagBits::eShaderDeviceAddress |
                   vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
        case rv::BufferUsage::Index:
            return vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                   vk::BufferUsageFlagBits::eStorageBuffer |
                   vk::BufferUsageFlagBits::eShaderDeviceAddress |
                   vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
        case rv::BufferUsage::Indirect:
            return vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst |
                   vk::BufferUsageFlagBits::eIndirectBuffer;
        case rv::BufferUsage::AccelStorage:
            return vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
                   vk::BufferUsageFlagBits::eShaderDeviceAddress;
        case rv::BufferUsage::AccelInput:
            return vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                   vk::BufferUsageFlagBits::eStorageBuffer |
                   vk::BufferUsageFlagBits::eShaderDeviceAddress |
                   vk::BufferUsageFlagBits::eTransferDst;
        case rv::BufferUsage::ShaderBindingTable:
            return vk::BufferUsageFlagBits::eShaderBindingTableKHR |
                   vk::BufferUsageFlagBits::eShaderDeviceAddress |
                   vk::BufferUsageFlagBits::eTransferDst;
        case rv::BufferUsage::Scratch:
            return vk::BufferUsageFlagBits::eStorageBuffer |
                   vk::BufferUsageFlagBits::eShaderDeviceAddress;
    }
}

vk::MemoryPropertyFlags getMemoryProperty(rv::MemoryUsage usage) {
    switch (usage) {
        case rv::MemoryUsage::Device:
            return vk::MemoryPropertyFlagBits::eDeviceLocal;
        case rv::MemoryUsage::Host:
            return vk::MemoryPropertyFlagBits::eHostVisible |
                   vk::MemoryPropertyFlagBits::eHostCoherent;
    }
}
}  // namespace

namespace rv {
Buffer::Buffer(const Context* context, BufferCreateInfo createInfo)
    : context{context}, size(createInfo.size), memoryUsage(createInfo.memory) {
    vk::BufferCreateInfo bufferInfo;
    bufferInfo.setSize(createInfo.size);
    bufferInfo.setUsage(getBufferUsage(createInfo.usage));
    buffer = context->getDevice().createBufferUnique(bufferInfo);

    vk::MemoryPropertyFlags memoryProperty = getMemoryProperty(createInfo.memory);
    vk::MemoryRequirements requirements = context->getDevice().getBufferMemoryRequirements(*buffer);
    uint32_t memoryTypeIndex = context->findMemoryTypeIndex(requirements, memoryProperty);

    vk::MemoryAllocateFlagsInfo flagsInfo{vk::MemoryAllocateFlagBits::eDeviceAddress};
    vk::MemoryAllocateInfo memoryInfo;
    memoryInfo.setAllocationSize(requirements.size);
    memoryInfo.setMemoryTypeIndex(memoryTypeIndex);
    memoryInfo.setPNext(&flagsInfo);
    memory = context->getDevice().allocateMemoryUnique(memoryInfo);

    context->getDevice().bindBufferMemory(*buffer, *memory, 0);

    if (createInfo.data) {
        copy(createInfo.data);
    }
}

void* Buffer::map() {
    RV_ASSERT(memoryUsage == MemoryUsage::Host, "");
    if (!mapped) {
        mapped = context->getDevice().mapMemory(*memory, 0, VK_WHOLE_SIZE);
    }
    return mapped;
}

void Buffer::unmap() {
    RV_ASSERT(memoryUsage == MemoryUsage::Host, "");
    context->getDevice().unmapMemory(*memory);
    mapped = nullptr;
}

void Buffer::copy(const void* data) {
    if (memoryUsage == MemoryUsage::Host) {
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
