#pragma once
#include "Vulkan.hpp"

class Buffer
{
public:
    void Init(size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProp);
    void InitOnHost(size_t size, vk::BufferUsageFlags usage);
    void InitOnDevice(size_t size, vk::BufferUsageFlags usage);
    void Copy(const void* data);
    vk::Buffer GetBuffer() const { return *buffer; }
    vk::DeviceSize GetSize() const { return size; }
    uint64_t GetAddress() const { return deviceAddress; }

    template <typename T>
    void InitOnHost(vk::BufferUsageFlags usage, std::vector<T> data)
    {
        InitOnHost(sizeof(T) * data.size(), usage);
        Copy(data.data());
    }

private:
    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;
    vk::DeviceSize size;
    uint64_t deviceAddress;
    void* mapped = nullptr;
};
