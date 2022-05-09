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

    template <typename T>
    void InitOnDevice(vk::BufferUsageFlags usage, std::vector<T> data)
    {
        size_t size = sizeof(T) * data.size();
        InitOnDevice(size, usage | vk::BufferUsageFlagBits::eTransferDst);

        Buffer staginBuffer;
        staginBuffer.InitOnHost(usage | vk::BufferUsageFlagBits::eTransferSrc, data);
        Vulkan::OneTimeSubmit(
            [&](vk::CommandBuffer cmdBuf)
            {
                vk::BufferCopy region{ 0, 0, size };
                cmdBuf.copyBuffer(staginBuffer.GetBuffer(), *buffer, region);
            });
    }

private:
    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;
    vk::DeviceSize size;
    uint64_t deviceAddress;
    void* mapped = nullptr;
};
