#pragma once
#include "Context.hpp"

namespace rv {
struct BufferCreateInfo {
    vk::BufferUsageFlags usage;
    vk::MemoryPropertyFlags memory;
    size_t size = 0;
    const void* data = nullptr;
    const char* debugName = nullptr;
};

class Buffer {
public:
    Buffer(const Context* context,
           vk::BufferUsageFlags usage,
           vk::MemoryPropertyFlags memoryProp,
           vk::DeviceSize size,
           const void* data,
           const char* debugName);

    vk::Buffer getBuffer() const { return *buffer; }

    vk::DeviceSize getSize() const { return size; }

    vk::DescriptorBufferInfo getInfo() const { return {*buffer, 0, size}; }

    vk::DeviceAddress getAddress() const;

    void* map();

    void unmap();

    void copy(const void* data);

private:
    const Context* context = nullptr;
    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;
    vk::DeviceSize size = 0u;
    void* mapped = nullptr;
    bool isHostVisible;
};
}  // namespace rv
