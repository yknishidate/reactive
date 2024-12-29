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

    auto getBuffer() const -> vk::Buffer { return *m_buffer; }
    auto getSize() const -> vk::DeviceSize { return m_size; }
    auto getInfo() const -> vk::DescriptorBufferInfo { return {*m_buffer, 0, m_size}; }
    auto getAddress() const -> vk::DeviceAddress;

    auto map() -> void*;
    void unmap();
    void copy(const void* data);

    void prepareStagingBuffer();

private:
    const Context* m_context = nullptr;

    vk::UniqueBuffer m_buffer;
    vk::UniqueDeviceMemory m_memory;
    vk::DeviceSize m_size = 0u;

    // For host buffer
    void* m_mapped = nullptr;
    bool m_isHostVisible;

    // For device buffer
    BufferHandle m_stagingBuffer;
};
}  // namespace rv
