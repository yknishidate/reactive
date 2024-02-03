#pragma once
#include "Context.hpp"

namespace rv {
struct BufferCreateInfo {
    vk::BufferUsageFlags usage;

    vk::MemoryPropertyFlags memory;

    size_t size = 0;

    std::string debugName;
};

class Buffer {
    friend class CommandBuffer;

public:
    Buffer(const Context& context, const BufferCreateInfo& createInfo);

    auto getBuffer() const -> vk::Buffer { return *buffer; }
    auto getSize() const -> vk::DeviceSize { return size; }
    auto getInfo() const -> vk::DescriptorBufferInfo { return {*buffer, 0, size}; }
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
