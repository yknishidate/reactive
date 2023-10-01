#pragma once
#include "Context.hpp"

namespace rv {
struct BufferCreateInfo {
    vk::BufferUsageFlags usage;
    vk::MemoryPropertyFlags memory;

    size_t size = 0;

    const char* debugName = nullptr;
};

class Buffer {
    friend class CommandBuffer;

public:
    Buffer(const Context* context,
           vk::BufferUsageFlags usage,
           vk::MemoryPropertyFlags memoryProp,
           vk::DeviceSize size,
           const char* debugName);

    auto getBuffer() const -> vk::Buffer;
    auto getSize() const -> vk::DeviceSize;
    auto getInfo() const -> vk::DescriptorBufferInfo;
    auto getAddress() const -> vk::DeviceAddress;

    auto map() -> void*;
    void unmap();
    void copy(const void* data);

    void prepareStagingBuffer();

private:
    const Context* context = nullptr;

    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;
    vk::DeviceSize size = 0u;

    // For host buffer
    void* mapped = nullptr;
    bool isHostVisible;

    // For device buffer
    BufferHandle stagingBuffer;
};
}  // namespace rv
