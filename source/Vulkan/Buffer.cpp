#include "Vulkan.hpp"
#include "Buffer.hpp"

namespace
{
    vk::UniqueBuffer createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage)
    {
        return Vulkan::Device.createBufferUnique({ {}, size, usage });
    }

    vk::UniqueDeviceMemory AllocateMemory(vk::Buffer buffer, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProp)
    {
        vk::MemoryRequirements requirements = Vulkan::Device.getBufferMemoryRequirements(buffer);
        uint32_t memoryTypeIndex = Vulkan::FindMemoryTypeIndex(requirements, memoryProp);
        vk::MemoryAllocateInfo memoryAllocateInfo;
        memoryAllocateInfo.setAllocationSize(requirements.size);
        memoryAllocateInfo.setMemoryTypeIndex(memoryTypeIndex);
        if (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
            vk::MemoryAllocateFlagsInfo flagsInfo{ vk::MemoryAllocateFlagBits::eDeviceAddress };
            memoryAllocateInfo.pNext = &flagsInfo;
            return Vulkan::Device.allocateMemoryUnique(memoryAllocateInfo);
        }
        return Vulkan::Device.allocateMemoryUnique(memoryAllocateInfo);
    }
}

void Buffer::Init(vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProp, size_t size)
{
    this->size = size;
    buffer = createBuffer(size, usage);
    memory = AllocateMemory(*buffer, usage, memoryProp);
    Vulkan::Device.bindBufferMemory(*buffer, *memory, 0);
    if (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
        vk::BufferDeviceAddressInfoKHR bufferDeviceAI{ *buffer };
        deviceAddress = Vulkan::Device.getBufferAddressKHR(&bufferDeviceAI);
    }
}

void HostBuffer::Init(vk::BufferUsageFlags usage, size_t size)
{
    Buffer::Init(usage, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, size);
}

void HostBuffer::Copy(const void* data)
{
    if (!mapped) {
        mapped = Vulkan::Device.mapMemory(*memory, 0, size);
    }
    std::memcpy(mapped, data, static_cast<size_t>(size));
}

void DeviceBuffer::Init(vk::BufferUsageFlags usage, size_t size)
{
    this->usage = usage;
    Buffer::Init(usage, vk::MemoryPropertyFlagBits::eDeviceLocal, size);
}

void DeviceBuffer::Copy(const void* data)
{
    HostBuffer staginBuffer;
    staginBuffer.Init(usage | vk::BufferUsageFlagBits::eTransferSrc, size);
    staginBuffer.Copy(data);

    Vulkan::OneTimeSubmit(
        [&](vk::CommandBuffer commandBuffer)
        {
            vk::BufferCopy region{ 0, 0, size };
            commandBuffer.copyBuffer(staginBuffer.GetBuffer(), *buffer, region);
        });
}
