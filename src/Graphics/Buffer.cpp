#include "reactive/Graphics/Buffer.hpp"

#include "reactive/Graphics/CommandBuffer.hpp"
#include "reactive/common.hpp"

namespace {
VmaMemoryUsage getVmaMemoryUsage(rv::BufferMemoryUsage usage) {
    switch (usage) {
        case rv::BufferMemoryUsage::Auto:
            return VMA_MEMORY_USAGE_AUTO;
        case rv::BufferMemoryUsage::DeviceLocal:
            return VMA_MEMORY_USAGE_GPU_ONLY;
        case rv::BufferMemoryUsage::HostVisible:
            return VMA_MEMORY_USAGE_CPU_ONLY;
        case rv::BufferMemoryUsage::HostCoherent:
            return VMA_MEMORY_USAGE_CPU_TO_GPU;
        default:
            return VMA_MEMORY_USAGE_AUTO;
    }
}
}

namespace rv {
Buffer::Buffer(const Context& context, const BufferCreateInfo& createInfo)
    : m_context{&context}, m_size{createInfo.size} {
    // Create buffer with VMA
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = m_size;
    bufferInfo.usage = static_cast<VkBufferUsageFlags>(createInfo.usage);

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = getVmaMemoryUsage(createInfo.memoryUsage);
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    
    // Enable device address if usage includes it
    if (createInfo.usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
        VkMemoryAllocateFlagsInfo flagsInfo{};
        flagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        flagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
        allocInfo.pUserData = &flagsInfo;
    }

    VkBuffer vkBuffer;
    vmaCreateBuffer(m_context->getAllocator(), &bufferInfo, &allocInfo, &vkBuffer, &m_allocation, &m_allocationInfo);
    m_buffer = vk::Buffer(vkBuffer);
    
    m_isHostVisible = (m_allocationInfo.pMappedData != nullptr);
    m_mapped = m_allocationInfo.pMappedData;

    if (!createInfo.debugName.empty()) {
        m_context->setDebugName(m_buffer, createInfo.debugName.c_str());
        vmaSetAllocationName(m_context->getAllocator(), m_allocation, createInfo.debugName.c_str());
    }
}

Buffer::~Buffer() {
    if (m_buffer) {
        vmaDestroyBuffer(m_context->getAllocator(), VkBuffer(m_buffer), m_allocation);
    }
}

auto Buffer::getAddress() const -> vk::DeviceAddress {
    vk::BufferDeviceAddressInfo addressInfo{m_buffer};
    return m_context->getDevice().getBufferAddress(&addressInfo);
}

auto Buffer::map() -> void* {
    RV_ASSERT(m_isHostVisible, "");
    if (!m_mapped) {
        vmaMapMemory(m_context->getAllocator(), m_allocation, &m_mapped);
    }
    return m_mapped;
}

void Buffer::unmap() {
    RV_ASSERT(m_isHostVisible, "This m_buffer is not host visible.");
    if (m_mapped && !m_allocationInfo.pMappedData) {
        vmaUnmapMemory(m_context->getAllocator(), m_allocation);
        m_mapped = nullptr;
    }
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
            .memoryUsage = BufferMemoryUsage::HostVisible,
            .size = m_size,
        });
    }
}
}  // namespace rv
