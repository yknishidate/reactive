#include "Context.hpp"
#include "Buffer.hpp"

namespace
{
    vk::UniqueBuffer CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage)
    {
        return Context::GetDevice().createBufferUnique({ {}, size, usage });
    }

    vk::UniqueDeviceMemory AllocateMemory(vk::Buffer buffer, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProp)
    {
        vk::MemoryRequirements requirements = Context::GetDevice().getBufferMemoryRequirements(buffer);
        uint32_t memoryTypeIndex = Context::FindMemoryTypeIndex(requirements, memoryProp);
        vk::MemoryAllocateInfo memoryAllocateInfo;
        memoryAllocateInfo.setAllocationSize(requirements.size);
        memoryAllocateInfo.setMemoryTypeIndex(memoryTypeIndex);
        if (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
            vk::MemoryAllocateFlagsInfo flagsInfo{ vk::MemoryAllocateFlagBits::eDeviceAddress };
            memoryAllocateInfo.pNext = &flagsInfo;
            return Context::GetDevice().allocateMemoryUnique(memoryAllocateInfo);
        }
        return Context::GetDevice().allocateMemoryUnique(memoryAllocateInfo);
    }
}

Buffer::Buffer(vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProp, size_t size)
{
    this->size = size;
    buffer = CreateBuffer(size, usage);
    memory = AllocateMemory(*buffer, usage, memoryProp);
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
    : usage{ usage }
    , Buffer(usage, vk::MemoryPropertyFlagBits::eDeviceLocal, size)
{
}

void DeviceBuffer::Copy(const void* data)
{
    HostBuffer staginBuffer{ usage | vk::BufferUsageFlagBits::eTransferSrc, size };
    staginBuffer.Copy(data);

    Context::OneTimeSubmit(
        [&](vk::CommandBuffer commandBuffer)
        {
            vk::BufferCopy region{ 0, 0, size };
            commandBuffer.copyBuffer(staginBuffer.GetBuffer(), *buffer, region);
        });
}
