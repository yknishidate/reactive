#pragma once
#include "Context.hpp"

class Buffer
{
public:
    Buffer() = default;
    virtual void Copy(const void* data) = 0;

    vk::Buffer GetBuffer() const { return *buffer; }
    vk::DeviceSize GetSize() const { return size; }
    uint64_t GetAddress() const { return deviceAddress; }

protected:
    Buffer(vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProp, size_t size);

    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;
    vk::DeviceSize size;
    uint64_t deviceAddress;
    void* mapped = nullptr;
};

class HostBuffer : public Buffer
{
public:
    HostBuffer() = default;
    HostBuffer(vk::BufferUsageFlags usage, size_t size);

    template <typename T>
    HostBuffer(vk::BufferUsageFlags usage, const std::vector<T>& data)
    {
        Init(usage, sizeof(T) * data.size());
        Copy(data.data());
    }

    void Copy(const void* data) override;
};

class DeviceBuffer : public Buffer
{
public:
    DeviceBuffer() = default;
    DeviceBuffer(vk::BufferUsageFlags usage, size_t size);

    template <typename T>
    DeviceBuffer(vk::BufferUsageFlags usage, const std::vector<T>& data)
        : DeviceBuffer(usage | vk::BufferUsageFlagBits::eTransferDst, sizeof(T)* data.size())
    {
        Copy(data.data());
    }

    void Copy(const void* data) override;

private:
    vk::BufferUsageFlags usage;
};
