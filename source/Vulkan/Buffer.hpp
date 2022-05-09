#pragma once
#include "Vulkan.hpp"

class Buffer
{
public:
    virtual void Init(vk::BufferUsageFlags usage, size_t size) = 0;
    virtual void Copy(const void* data) = 0;

    vk::Buffer GetBuffer() const { return *buffer; }
    vk::DeviceSize GetSize() const { return size; }
    uint64_t GetAddress() const { return deviceAddress; }

protected:
    void Init(vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProp, size_t size);

    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;
    vk::DeviceSize size;
    uint64_t deviceAddress;
    void* mapped = nullptr;
};

class HostBuffer : public Buffer
{
public:
    void Init(vk::BufferUsageFlags usage, size_t size) override;
    void Copy(const void* data) override;

    template <typename T>
    void Init(vk::BufferUsageFlags usage, std::vector<T> data)
    {
        Init(usage, sizeof(T) * data.size());
        Copy(data.data());
    }
};

class DeviceBuffer : public Buffer
{
public:
    void Init(vk::BufferUsageFlags usage, size_t size) override;
    void Copy(const void* data) override;

    template <typename T>
    void Init(vk::BufferUsageFlags usage, std::vector<T> data)
    {
        size_t size = sizeof(T) * data.size();
        Init(usage | vk::BufferUsageFlagBits::eTransferDst, size);
        Copy(data.data());
    }

private:
    vk::BufferUsageFlags usage;
};
