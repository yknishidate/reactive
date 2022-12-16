#pragma once
#include "Graphics.hpp"

class Buffer
{
public:
    using Usage = vk::BufferUsageFlagBits;
    Buffer() = default;

    vk::Buffer GetBuffer() const { return *buffer; }
    vk::DeviceSize GetSize() const { return size; }
    uint64_t GetAddress() const { return deviceAddress; }
    vk::DescriptorBufferInfo GetInfo() const { return { *buffer, 0, size }; }

protected:
    Buffer(vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProp, size_t size);

    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;
    vk::DeviceSize size = 0u;
    uint64_t deviceAddress = 0u;
};

class HostBuffer : public Buffer
{
public:
    HostBuffer() = default;
    HostBuffer(vk::BufferUsageFlags usage, size_t size);

    void Copy(const void* data);
    void* Map();

private:
    void* mapped = nullptr;
};

class DeviceBuffer : public Buffer
{
public:
    DeviceBuffer() = default;
    DeviceBuffer(vk::BufferUsageFlags usage, size_t size);

    template <typename T>
    DeviceBuffer(vk::BufferUsageFlags usage, const std::vector<T>& data)
        : DeviceBuffer(usage | Usage::eTransferDst, sizeof(T)* data.size())
    {
        Copy(data.data());
    }

    void Copy(const void* data);
};

class StagingBuffer : public HostBuffer
{
public:
    StagingBuffer() = default;
    StagingBuffer(size_t size, const void* data);
};

class StorageBuffer : public DeviceBuffer
{
public:
    StorageBuffer() = default;
    StorageBuffer(size_t size)
        : DeviceBuffer(Usage::eStorageBuffer | Usage::eShaderDeviceAddress, size)
    {
    }

    template <typename T>
    StorageBuffer(const std::vector<T>& data)
        : DeviceBuffer(Usage::eStorageBuffer | Usage::eShaderDeviceAddress | Usage::eTransferDst, sizeof(T)* data.size())
    {
        Copy(data.data());
    }
};
