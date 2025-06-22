#include "reactive/Graphics/MemoryManager.hpp"
#include "reactive/Graphics/Context.hpp"

namespace rv {

MemoryManager::MemoryManager(const Context* context) : m_context(context) {
    RV_ASSERT(m_context != nullptr, "Context cannot be null");
    
    // VMA アロケーター作成情報
    VmaAllocatorCreateInfo createInfo{};
    createInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    createInfo.physicalDevice = m_context->getPhysicalDevice();
    createInfo.device = m_context->getDevice();
    createInfo.instance = m_context->getInstance();
    
    // デバイスアドレス機能が有効な場合のフラグ設定
    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
    bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    features2.pNext = &bufferDeviceAddressFeatures;
    
    vkGetPhysicalDeviceFeatures2(m_context->getPhysicalDevice(), &features2);
    
    if (bufferDeviceAddressFeatures.bufferDeviceAddress) {
        createInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    }
    
    // メモリマッピングの最適化
    createInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    
    VkResult result = vmaCreateAllocator(&createInfo, &m_allocator);
    RV_ASSERT(result == VK_SUCCESS, "Failed to create VMA allocator: {}", static_cast<int>(result));
    
    spdlog::info("VMA Memory Allocator initialized successfully");
}

MemoryManager::~MemoryManager() {
    if (m_allocator != VK_NULL_HANDLE) {
        vmaDestroyAllocator(m_allocator);
        spdlog::info("VMA Memory Allocator destroyed");
    }
}

BufferAllocation MemoryManager::createBuffer(const BufferCreateInfo& createInfo) {
    // Vulkan バッファー作成情報
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = createInfo.size;
    bufferInfo.usage = static_cast<VkBufferUsageFlags>(createInfo.usage);
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    // VMA 割り当て情報
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = convertMemoryUsage(createInfo.memoryUsage);
    allocInfo.flags = getMemoryFlags(createInfo.memoryUsage);
    
    if (createInfo.preferredFlags) {
        allocInfo.flags |= VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
    }
    
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;
    
    VkResult result = vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, 
                                     &buffer, &allocation, &allocationInfo);
    
    if (result != VK_SUCCESS) {
        spdlog::error("Failed to create buffer: {} (size: {} bytes)", 
                     static_cast<int>(result), createInfo.size);
        throw std::runtime_error("Failed to create buffer with VMA");
    }
    
    // デバッグ名設定
    if (!createInfo.debugName.empty()) {
        setDebugName(vk::Buffer(buffer), createInfo.debugName);
    }
    
    spdlog::debug("Created buffer: {} bytes, memory type: {}", 
                 createInfo.size, allocationInfo.memoryType);
    
    return BufferAllocation(vk::Buffer(buffer), allocation, allocationInfo);
}

void MemoryManager::destroyBuffer(const BufferAllocation& allocation) {
    if (allocation.buffer && allocation.allocation) {
        vmaDestroyBuffer(m_allocator, static_cast<VkBuffer>(allocation.buffer), allocation.allocation);
        spdlog::debug("Destroyed buffer");
    }
}

ImageAllocation MemoryManager::createImage(const ImageCreateInfo& createInfo) {
    // Vulkan イメージ作成情報
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = static_cast<VkImageType>(createInfo.imageType);
    imageInfo.extent.width = createInfo.extent.width;
    imageInfo.extent.height = createInfo.extent.height;
    imageInfo.extent.depth = createInfo.extent.depth;
    imageInfo.mipLevels = createInfo.mipLevels;
    imageInfo.arrayLayers = createInfo.arrayLayers;
    imageInfo.format = static_cast<VkFormat>(createInfo.format);
    imageInfo.tiling = static_cast<VkImageTiling>(createInfo.tiling);
    imageInfo.initialLayout = static_cast<VkImageLayout>(createInfo.initialLayout);
    imageInfo.usage = static_cast<VkImageUsageFlags>(createInfo.usage);
    imageInfo.samples = static_cast<VkSampleCountFlagBits>(createInfo.samples);
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    // VMA 割り当て情報
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = convertMemoryUsage(createInfo.memoryUsage);
    allocInfo.flags = getMemoryFlags(createInfo.memoryUsage);
    
    VkImage image;
    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;
    
    VkResult result = vmaCreateImage(m_allocator, &imageInfo, &allocInfo,
                                    &image, &allocation, &allocationInfo);
    
    if (result != VK_SUCCESS) {
        spdlog::error("Failed to create image: {} ({}x{}x{})", 
                     static_cast<int>(result), 
                     createInfo.extent.width, createInfo.extent.height, createInfo.extent.depth);
        throw std::runtime_error("Failed to create image with VMA");
    }
    
    // デバッグ名設定
    if (!createInfo.debugName.empty()) {
        setDebugName(vk::Image(image), createInfo.debugName);
    }
    
    spdlog::debug("Created image: {}x{}x{}, format: {}, memory type: {}", 
                 createInfo.extent.width, createInfo.extent.height, createInfo.extent.depth,
                 static_cast<int>(createInfo.format), allocationInfo.memoryType);
    
    return ImageAllocation(vk::Image(image), allocation, allocationInfo);
}

void MemoryManager::destroyImage(const ImageAllocation& allocation) {
    if (allocation.image && allocation.allocation) {
        vmaDestroyImage(m_allocator, static_cast<VkImage>(allocation.image), allocation.allocation);
        spdlog::debug("Destroyed image");
    }
}

void* MemoryManager::mapMemory(const BufferAllocation& allocation) {
    void* data;
    VkResult result = vmaMapMemory(m_allocator, allocation.allocation, &data);
    
    if (result != VK_SUCCESS) {
        spdlog::error("Failed to map memory: {}", static_cast<int>(result));
        throw std::runtime_error("Failed to map memory");
    }
    
    return data;
}

void MemoryManager::unmapMemory(const BufferAllocation& allocation) {
    vmaUnmapMemory(m_allocator, allocation.allocation);
}

void MemoryManager::getMemoryStatistics(VmaTotalStatistics* stats) const {
    RV_ASSERT(stats != nullptr, "Stats pointer cannot be null");
    vmaCalculateStatistics(m_allocator, stats);
}

void MemoryManager::getMemoryBudget(VmaBudget* budget) const {
    RV_ASSERT(budget != nullptr, "Budget pointer cannot be null");
    vmaGetHeapBudgets(m_allocator, budget);
}

void MemoryManager::defragment() {
    VmaDefragmentationInfo defragInfo{};
    defragInfo.flags = VMA_DEFRAGMENTATION_FLAG_ALGORITHM_BALANCED_BIT;
    
    VmaDefragmentationContext context;
    VkResult result = vmaBeginDefragmentation(m_allocator, &defragInfo, &context);
    
    if (result == VK_SUCCESS) {
        // デフラグメンテーション実行
        VmaDefragmentationPassMoveInfo passInfo{};
        result = vmaBeginDefragmentationPass(m_allocator, context, &passInfo);
        
        if (result == VK_SUCCESS) {
            vmaEndDefragmentationPass(m_allocator, context, &passInfo);
            spdlog::info("Memory defragmentation completed");
        }
        
        VmaDefragmentationStats stats;
        vmaEndDefragmentation(m_allocator, context, &stats);
        
        spdlog::info("Defragmentation stats: {} bytes moved, {} allocations moved", 
                    stats.bytesMoved, stats.allocationsMoved);
    }
}

void MemoryManager::dumpMemoryToJson(const std::string& filePath) const {
    char* json;
    vmaBuildStatsString(m_allocator, &json, VK_TRUE);
    
    std::ofstream file(filePath);
    if (file.is_open()) {
        file << json;
        file.close();
        spdlog::info("Memory statistics dumped to: {}", filePath);
    } else {
        spdlog::error("Failed to write memory statistics to: {}", filePath);
    }
    
    vmaFreeStatsString(m_allocator, json);
}

VmaMemoryUsage MemoryManager::convertMemoryUsage(MemoryUsage usage) const {
    switch (usage) {
        case MemoryUsage::GpuOnly:
            return VMA_MEMORY_USAGE_GPU_ONLY;
        case MemoryUsage::CpuOnly:
            return VMA_MEMORY_USAGE_CPU_ONLY;
        case MemoryUsage::CpuToGpu:
            return VMA_MEMORY_USAGE_CPU_TO_GPU;
        case MemoryUsage::GpuToCpu:
            return VMA_MEMORY_USAGE_GPU_TO_CPU;
        case MemoryUsage::CpuCopy:
            return VMA_MEMORY_USAGE_CPU_COPY;
        case MemoryUsage::GpuLazilyAllocated:
            return VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
        default:
            return VMA_MEMORY_USAGE_GPU_ONLY;
    }
}

VmaAllocationCreateFlags MemoryManager::getMemoryFlags(MemoryUsage usage) const {
    VmaAllocationCreateFlags flags = 0;
    
    switch (usage) {
        case MemoryUsage::CpuToGpu:
        case MemoryUsage::CpuCopy:
            flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
            break;
        case MemoryUsage::GpuToCpu:
            flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
            break;
        case MemoryUsage::CpuOnly:
            flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
            break;
        default:
            break;
    }
    
    return flags;
}

void MemoryManager::setDebugName(vk::Buffer buffer, const std::string& name) const {
    if (m_context->isDebugUtilsEnabled()) {
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectType = vk::ObjectType::eBuffer;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(static_cast<VkBuffer>(buffer));
        nameInfo.pObjectName = name.c_str();
        
        auto result = m_context->getDevice().setDebugUtilsObjectNameEXT(&nameInfo);
        if (result != vk::Result::eSuccess) {
            spdlog::warn("Failed to set debug name for buffer: {}", name);
        }
    }
}

void MemoryManager::setDebugName(vk::Image image, const std::string& name) const {
    if (m_context->isDebugUtilsEnabled()) {
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectType = vk::ObjectType::eImage;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(static_cast<VkImage>(image));
        nameInfo.pObjectName = name.c_str();
        
        auto result = m_context->getDevice().setDebugUtilsObjectNameEXT(&nameInfo);
        if (result != vk::Result::eSuccess) {
            spdlog::warn("Failed to set debug name for image: {}", name);
        }
    }
}

} // namespace rv