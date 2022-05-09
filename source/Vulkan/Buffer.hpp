#pragma once
#include "Vulkan.hpp"

class Buffer
{
public:
    void Init(vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProp, size_t size);
    vk::Buffer GetBuffer() const { return *buffer; }
    vk::DeviceSize GetSize() const { return size; }
    uint64_t GetAddress() const { return deviceAddress; }

protected:
    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;
    vk::DeviceSize size;
    uint64_t deviceAddress;
    void* mapped = nullptr;
};

class HostBuffer : public Buffer
{
public:
    void Init(vk::BufferUsageFlags usage, size_t size);

    template <typename T>
    void Init(vk::BufferUsageFlags usage, std::vector<T> data)
    {
        Init(usage, sizeof(T) * data.size());
        Copy(data.data());
    }

    void Copy(const void* data);
};

class DeviceBuffer : public Buffer
{
public:
    void Init(vk::BufferUsageFlags usage, size_t size);

    template <typename T>
    void Init(vk::BufferUsageFlags usage, std::vector<T> data)
    {
        size_t size = sizeof(T) * data.size();
        Init(usage | vk::BufferUsageFlagBits::eTransferDst, size);
        Copy(data.data());
        //HostBuffer staginBuffer;
        //staginBuffer.Init(usage | vk::BufferUsageFlagBits::eTransferSrc, data);

        //Vulkan::OneTimeSubmit(
        //    [&](vk::CommandBuffer commandBuffer)
        //    {
        //        vk::BufferCopy region{ 0, 0, size };
        //        commandBuffer.copyBuffer(staginBuffer.GetBuffer(), *buffer, region);
        //    });
    }

    void Copy(const void* data);

private:
    vk::BufferUsageFlags usage;
};
