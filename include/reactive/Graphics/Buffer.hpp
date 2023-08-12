#pragma once
#include "Context.hpp"

namespace rv {
class Buffer {
public:
    Buffer(const Context* context,
           vk::BufferUsageFlags usage,
           vk::MemoryPropertyFlags memoryProp,
           vk::DeviceSize size,
           const void* data);

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
