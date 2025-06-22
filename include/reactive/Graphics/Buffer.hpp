#pragma once
#include "Context.hpp"
#include "MemoryManager.hpp"

namespace rv {
struct BufferCreateInfo {
    vk::BufferUsageFlags usage;
    size_t size = 0;
    MemoryUsage memoryUsage = MemoryUsage::GpuOnly;
    std::string debugName;
};

class Buffer {
    friend class CommandBuffer;

public:
    Buffer(const Context& context, const BufferCreateInfo& createInfo);
    ~Buffer();

    // コピー・ムーブ禁止（VMA割り当てのため）
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&&) = delete;
    Buffer& operator=(Buffer&&) = delete;

    auto getBuffer() const -> vk::Buffer { 
        return m_vmaAllocation.buffer; 
    }
    auto getSize() const -> vk::DeviceSize { return m_size; }
    auto getInfo() const -> vk::DescriptorBufferInfo { 
        return {getBuffer(), 0, m_size}; 
    }
    auto getAddress() const -> vk::DeviceAddress;

    auto map() -> void*;
    void unmap();
    void copy(const void* data);

    void prepareStagingBuffer();
    
    // VMA関連のメソッド
    const BufferAllocation& getAllocation() const { return m_vmaAllocation; }

private:
    const Context* m_context = nullptr;
    vk::DeviceSize m_size = 0u;
    BufferAllocation m_vmaAllocation;
    
    // ホストバッファー用
    void* m_mapped = nullptr;
    bool m_isHostVisible = false;

    // デバイスバッファー用
    BufferHandle m_stagingBuffer;
};
}  // namespace rv
