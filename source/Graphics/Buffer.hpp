#pragma once
#include "Context.hpp"

enum class BufferUsage {
    Vertex,
    Index,
    AccelInput,
    AccelStorage,
    Scratch,
    ShaderBindingTable,
    Staging,
    Storage,
};

class Buffer
{
public:
    Buffer() = default;

    vk::Buffer getBuffer() const { return *buffer; }
    vk::DeviceSize getSize() const { return size; }
    uint64_t getAddress() const {
        vk::BufferDeviceAddressInfoKHR bufferDeviceAI{ *buffer };
        return Context::getDevice().getBufferAddressKHR(&bufferDeviceAI);
    }
    vk::DescriptorBufferInfo getInfo() const { return { *buffer, 0, size }; }

protected:
    Buffer(BufferUsage usage, vk::MemoryPropertyFlags memoryProp, size_t size);

    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;
    vk::DeviceSize size = 0u;
};

class HostBuffer : public Buffer
{
public:
    HostBuffer() = default;
    HostBuffer(BufferUsage usage, size_t size);

    void copy(const void* data);
    void* map();

private:
    void* mapped = nullptr;
};

class DeviceBuffer : public Buffer
{
public:
    DeviceBuffer() = default;
    DeviceBuffer(BufferUsage usage, size_t size);

    template <typename T>
    DeviceBuffer(BufferUsage usage, const std::vector<T>& data)
        : DeviceBuffer(usage, sizeof(T)* data.size())
    {
        copy(data.data());
    }

    void copy(const void* data);
};
