#include "Buffer.hpp"

Buffer::Buffer(vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProp, size_t size)
    : size(size)
{
    buffer = Context::getDevice().createBufferUnique({ {}, size, usage });

    vk::MemoryRequirements requirements = Context::getDevice().getBufferMemoryRequirements(*buffer);
    uint32_t memoryTypeIndex = Context::findMemoryTypeIndex(requirements, memoryProp);
    vk::MemoryAllocateFlagsInfo flagsInfo{ vk::MemoryAllocateFlagBits::eDeviceAddress };
    memory = Context::getDevice().allocateMemoryUnique(
        vk::MemoryAllocateInfo()
        .setAllocationSize(requirements.size)
        .setMemoryTypeIndex(memoryTypeIndex)
        .setPNext(&flagsInfo));

    Context::getDevice().bindBufferMemory(*buffer, *memory, 0);

    if (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
        vk::BufferDeviceAddressInfoKHR bufferDeviceAI{ *buffer };
        deviceAddress = Context::getDevice().getBufferAddressKHR(&bufferDeviceAI);
    }
}

HostBuffer::HostBuffer(vk::BufferUsageFlags usage, size_t size)
    : Buffer(usage, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, size)
{
}

void HostBuffer::copy(const void* data)
{
    map();
    std::memcpy(mapped, data, size);
}

void* HostBuffer::map()
{
    if (!mapped) {
        mapped = Context::getDevice().mapMemory(*memory, 0, VK_WHOLE_SIZE);
    }
    return mapped;
}

DeviceBuffer::DeviceBuffer(vk::BufferUsageFlags usage, size_t size)
    : Buffer(usage, vk::MemoryPropertyFlagBits::eDeviceLocal, size)
{
}

void DeviceBuffer::copy(const void* data)
{
    HostBuffer stagingBuffer{ Usage::eTransferSrc, size };
    stagingBuffer.copy(data);
    Context::oneTimeSubmit([&](vk::CommandBuffer commandBuffer) {
        vk::BufferCopy region{ 0, 0, size };
    commandBuffer.copyBuffer(stagingBuffer.getBuffer(), *buffer, region);
    });
}
