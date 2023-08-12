#pragma once
#include "Context.hpp"

namespace rv {
enum class BufferUsage {
    Uniform,
    Storage,
    Staging,
    Vertex,
    Index,
    Indirect,
    AccelStorage,
    AccelInput,
    ShaderBindingTable,
    Scratch,
};

enum class MemoryUsage {
    Device,
    Host,
};

struct BufferCreateInfo {
    BufferUsage usage;
    MemoryUsage memory;
    size_t size = 0;
    const void* data = nullptr;
};

class Buffer {
public:
    Buffer() = default;

    Buffer(const Context* context, BufferCreateInfo createInfo);

    vk::Buffer getBuffer() const { return *buffer; }

    vk::DeviceSize getSize() const { return size; }

    vk::DescriptorBufferInfo getInfo() const { return {*buffer, 0, size}; }

    uint64_t getAddress() const {
        vk::BufferDeviceAddressInfo bufferDeviceAI{*buffer};
        return context->getDevice().getBufferAddress(&bufferDeviceAI);
    }

    void* map();

    void unmap();

    void copy(const void* data);

protected:
    Buffer(const Context* context,
           BufferUsage usage,
           vk::MemoryPropertyFlags memoryProp,
           size_t size);

    const Context* context = nullptr;
    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;
    vk::DeviceSize size = 0u;
    void* mapped = nullptr;
    MemoryUsage memoryUsage;
};
}  // namespace rv
