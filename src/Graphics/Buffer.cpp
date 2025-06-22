#include "reactive/Graphics/Buffer.hpp"
#include "reactive/Graphics/CommandBuffer.hpp"
#include "reactive/common.hpp"

namespace rv {

Buffer::Buffer(const Context& context, const BufferCreateInfo& createInfo)
    : m_context{&context}, m_size{createInfo.size} {
    
    // VMA を使用したメモリ管理
    rv::BufferCreateInfo vmaCreateInfo{
        .size = static_cast<vk::DeviceSize>(m_size),
        .usage = createInfo.usage,
        .memoryUsage = createInfo.memoryUsage,
        .debugName = createInfo.debugName
    };
    
    m_vmaAllocation = m_context->getMemoryManager().createBuffer(vmaCreateInfo);
    
    // ホストアクセス可能かどうかを判定
    m_isHostVisible = (createInfo.memoryUsage == MemoryUsage::CpuOnly || 
                      createInfo.memoryUsage == MemoryUsage::CpuToGpu ||
                      createInfo.memoryUsage == MemoryUsage::GpuToCpu ||
                      createInfo.memoryUsage == MemoryUsage::CpuCopy);
    
    // 既にマップされている場合はポインタを保存
    if (m_vmaAllocation.isMapped()) {
        m_mapped = m_vmaAllocation.getMappedData();
    }
    
    spdlog::debug("Created VMA buffer: {} bytes, usage: {}, memory: {}", 
                 m_size, static_cast<uint32_t>(createInfo.usage), static_cast<int>(createInfo.memoryUsage));
}

Buffer::~Buffer() {
    if (m_vmaAllocation.buffer) {
        m_context->getMemoryManager().destroyBuffer(m_vmaAllocation);
        spdlog::debug("Destroyed VMA buffer");
    }
}

auto Buffer::getAddress() const -> vk::DeviceAddress {
    vk::BufferDeviceAddressInfo addressInfo{getBuffer()};
    return m_context->getDevice().getBufferAddress(&addressInfo);
}

auto Buffer::map() -> void* {
    RV_ASSERT(m_isHostVisible, "Buffer is not host visible");
    
    if (!m_mapped) {
        m_mapped = const_cast<MemoryManager&>(m_context->getMemoryManager()).mapMemory(m_vmaAllocation);
    }
    return m_mapped;
}

void Buffer::unmap() {
    RV_ASSERT(m_isHostVisible, "Buffer is not host visible");
    
    if (m_mapped && !m_vmaAllocation.isMapped()) {
        const_cast<MemoryManager&>(m_context->getMemoryManager()).unmapMemory(m_vmaAllocation);
        m_mapped = nullptr;
    }
}

void Buffer::copy(const void* data) {
    RV_ASSERT(m_isHostVisible, "Buffer is not host visible");
    RV_ASSERT(data != nullptr, "Data pointer cannot be null");
    
    map();
    std::memcpy(m_mapped, data, m_size);
}

void Buffer::prepareStagingBuffer() {
    RV_ASSERT(!m_isHostVisible, "Buffer is already host visible");
    
    if (!m_stagingBuffer) {
        BufferCreateInfo stagingInfo{
            .usage = BufferUsage::Staging,
            .size = m_size,
            .memoryUsage = MemoryUsage::CpuToGpu,
            .debugName = "StagingBuffer_" + std::to_string(reinterpret_cast<uintptr_t>(this))
        };
        
        m_stagingBuffer = m_context->createBuffer(stagingInfo);
    }
}

} // namespace rv