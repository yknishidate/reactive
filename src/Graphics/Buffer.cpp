#include "reactive/Graphics/Buffer.hpp"

#include "reactive/Graphics/CommandBuffer.hpp"
#include "reactive/common.hpp"

namespace rv {
Buffer::Buffer(const Context& context, const BufferCreateInfo& createInfo)
    : m_context{&context}, m_size{createInfo.size} {
    // Create buffer
    vk::BufferCreateInfo bufferInfo;
    bufferInfo.setSize(m_size);
    bufferInfo.setUsage(createInfo.usage);
    m_buffer = m_context->getDevice().createBufferUnique(bufferInfo);

    // Allocate memory
    vk::MemoryRequirements requirements = m_context->getDevice().getBufferMemoryRequirements(*m_buffer);
    uint32_t memoryTypeIndex = m_context->findMemoryTypeIndex(requirements, createInfo.memory);

    vk::MemoryAllocateFlagsInfo flagsInfo{vk::MemoryAllocateFlagBits::eDeviceAddress};
    vk::MemoryAllocateInfo memoryInfo;
    memoryInfo.setAllocationSize(requirements.size);
    memoryInfo.setMemoryTypeIndex(memoryTypeIndex);
    memoryInfo.setPNext(&flagsInfo);
    m_memory = m_context->getDevice().allocateMemoryUnique(memoryInfo);

    m_isHostVisible = static_cast<bool>(createInfo.memory & vk::MemoryPropertyFlagBits::eHostVisible);

    // Bind memory
    m_context->getDevice().bindBufferMemory(*m_buffer, *m_memory, 0);

    if (!createInfo.debugName.empty()) {
        m_context->setDebugName(*m_buffer, createInfo.debugName.c_str());
        m_context->setDebugName(*m_memory, createInfo.debugName.c_str());
    }
}

auto Buffer::getAddress() const -> vk::DeviceAddress {
    vk::BufferDeviceAddressInfo addressInfo{*m_buffer};
    return m_context->getDevice().getBufferAddress(&addressInfo);
}

auto Buffer::map() -> void* {
    RV_ASSERT(m_isHostVisible, "");
    if (!m_mapped) {
        m_mapped = m_context->getDevice().mapMemory(*m_memory, 0, VK_WHOLE_SIZE);
    }
    return m_mapped;
}

void Buffer::unmap() {
    RV_ASSERT(m_isHostVisible, "This m_buffer is not host visible.");
    m_context->getDevice().unmapMemory(*m_memory);
    m_mapped = nullptr;
}

void Buffer::copy(const void* data) {
    RV_ASSERT(m_isHostVisible, "This m_buffer is not host visible.");
    map();
    std::memcpy(m_mapped, data, m_size);
}

void Buffer::prepareStagingBuffer() {
    RV_ASSERT(!m_isHostVisible, "This m_buffer is not m_device m_buffer.");
    if (!m_stagingBuffer) {
        m_stagingBuffer = m_context->createBuffer({
            .usage = BufferUsage::Staging,
            .memory = MemoryUsage::Host,
            .size = m_size,
        });
    }
}
}  // namespace rv
