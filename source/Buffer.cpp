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

void Buffer::Init(size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProp)
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

void Buffer::InitOnHost(size_t size, vk::BufferUsageFlags usage)
{
    Init(size, usage, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
}

void Buffer::InitOnDevice(size_t size, vk::BufferUsageFlags usage)
{
    Init(size, usage, vk::MemoryPropertyFlagBits::eDeviceLocal);
}

void Buffer::Copy(void* data)
{
    if (!mapped) {
        mapped = Vulkan::Device.mapMemory(*memory, 0, size);
    }
    std::memcpy(mapped, data, static_cast<size_t>(size));
}
