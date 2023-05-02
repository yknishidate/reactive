#pragma once
#include "App.hpp"

enum class BufferUsage {
    Vertex,
    Index,
    AccelInput,
    AccelStorage,
    Scratch,
    ShaderBindingTable,
    Staging,
    Storage,
    Uniform,
};

class Buffer {
public:
    Buffer() = default;

    vk::Buffer getBuffer() const { return *buffer; }
    vk::DeviceSize getSize() const { return size; }
    uint64_t getAddress() const {
        vk::BufferDeviceAddressInfoKHR bufferDeviceAI{*buffer};
        return context->getDevice().getBufferAddressKHR(&bufferDeviceAI);
    }
    vk::DescriptorBufferInfo getInfo() const { return {*buffer, 0, size}; }

protected:
    Buffer(const Context* context,
           BufferUsage usage,
           vk::MemoryPropertyFlags memoryProp,
           size_t size);

    const Context* context;
    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;
    vk::DeviceSize size = 0u;
};

class HostBuffer : public Buffer {
public:
    HostBuffer() = default;
    HostBuffer(const Context* context, BufferUsage usage, size_t size);

    void copy(const void* data);
    void* map();
    void unmap();

private:
    void* mapped = nullptr;
};

class DeviceBuffer : public Buffer {
public:
    DeviceBuffer() = default;
    DeviceBuffer(const Context* context, BufferUsage usage, size_t size);

    template <typename T>
    DeviceBuffer(const Context* context, BufferUsage usage, const std::vector<T>& data)
        : DeviceBuffer(context, usage, sizeof(T) * data.size()) {
        copy(data.data());
    }

    void copy(const void* data);
};
