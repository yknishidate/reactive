#include "Buffer.hpp"
#include "Helper.hpp"

Buffer::Buffer(vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProp, size_t size)
{
    this->size = size;

    buffer = Helper::CreateBuffer(Context::GetDevice(), size, usage);

    vk::MemoryRequirements requirements = Context::GetDevice().getBufferMemoryRequirements(*buffer);
    uint32_t memoryTypeIndex = Context::FindMemoryTypeIndex(requirements, memoryProp);
    vk::MemoryAllocateFlagsInfo flagsInfo{ vk::MemoryAllocateFlagBits::eDeviceAddress };
    memory = Helper::AllocateMemory(Context::GetDevice(), requirements.size, memoryTypeIndex, flagsInfo);

    Context::GetDevice().bindBufferMemory(*buffer, *memory, 0);

    if (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
        vk::BufferDeviceAddressInfoKHR bufferDeviceAI{ *buffer };
        deviceAddress = Context::GetDevice().getBufferAddressKHR(&bufferDeviceAI);
    }
}

HostBuffer::HostBuffer(vk::BufferUsageFlags usage, size_t size)
    : Buffer(usage, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, size)
{
}

void HostBuffer::Copy(const void* data)
{
    if (!mapped) {
        mapped = Context::GetDevice().mapMemory(*memory, 0, size);
    }
    std::memcpy(mapped, data, static_cast<size_t>(size));
}

DeviceBuffer::DeviceBuffer(vk::BufferUsageFlags usage, size_t size)
    : Buffer(usage, vk::MemoryPropertyFlagBits::eDeviceLocal, size)
{
}

void DeviceBuffer::Copy(const void* data)
{
    StagingBuffer stagingBuffer{ size, data };
    Context::OneTimeSubmit(
        [&](vk::CommandBuffer commandBuffer)
        {
            vk::BufferCopy region{ 0, 0, size };
            commandBuffer.copyBuffer(stagingBuffer.GetBuffer(), *buffer, region);
        });
}

StagingBuffer::StagingBuffer(size_t size, const void* data)
    : HostBuffer{ vk::BufferUsageFlagBits::eTransferSrc, size }
{
    Copy(data);
}
