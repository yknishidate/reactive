#pragma once
#include "reactive/common.hpp"

namespace rv {

// Forward declarations
class Context;

// メモリ割り当て情報
struct BufferAllocation {
    vk::Buffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
    
    BufferAllocation() = default;
    BufferAllocation(vk::Buffer buf, VmaAllocation alloc, const VmaAllocationInfo& allocInfo)
        : buffer(buf), allocation(alloc), info(allocInfo) {}
    
    // データマッピング用ヘルパー
    void* getMappedData() const { return info.pMappedData; }
    bool isMapped() const { return info.pMappedData != nullptr; }
};

struct ImageAllocation {
    vk::Image image;
    VmaAllocation allocation;
    VmaAllocationInfo info;
    
    ImageAllocation() = default;
    ImageAllocation(vk::Image img, VmaAllocation alloc, const VmaAllocationInfo& allocInfo)
        : image(img), allocation(alloc), info(allocInfo) {}
};

// メモリ使用方法を指定
enum class MemoryUsage {
    GpuOnly,           // GPU専用（Device Local）
    CpuOnly,           // CPU専用（Host Visible）
    CpuToGpu,          // CPU→GPU転送用（Host Visible + Host Coherent）
    GpuToCpu,          // GPU→CPU読み取り用（Host Visible + Host Cached）
    CpuCopy,           // CPU側コピー用（Host Visible + Host Coherent + Host Cached）
    GpuLazilyAllocated // 遅延割り当て（Lazily Allocated）
};

// バッファー作成情報
struct BufferCreateInfo {
    vk::DeviceSize size;
    vk::BufferUsageFlags usage;
    MemoryUsage memoryUsage = MemoryUsage::GpuOnly;
    bool preferredFlags = false;  // 推奨フラグを優先するか
    std::string debugName;
};

// イメージ作成情報
struct ImageCreateInfo {
    vk::ImageType imageType = vk::ImageType::e2D;
    vk::Format format;
    vk::Extent3D extent;
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;
    vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
    vk::ImageTiling tiling = vk::ImageTiling::eOptimal;
    vk::ImageUsageFlags usage;
    vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined;
    MemoryUsage memoryUsage = MemoryUsage::GpuOnly;
    std::string debugName;
};

/**
 * VMA (Vulkan Memory Allocator) を使用したメモリ管理クラス
 * 
 * このクラスは以下の問題を解決します：
 * 1. 個別VkDeviceMemory割り当てによる制限到達
 * 2. メモリフラグメンテーション
 * 3. メモリ使用量の最適化
 * 4. 自動的なメモリタイプ選択
 */
class MemoryManager {
public:
    explicit MemoryManager(const Context* context);
    ~MemoryManager();
    
    // コピー・ムーブ禁止
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;
    MemoryManager(MemoryManager&&) = delete;
    MemoryManager& operator=(MemoryManager&&) = delete;
    
    // バッファー作成・破棄
    BufferAllocation createBuffer(const BufferCreateInfo& createInfo);
    void destroyBuffer(const BufferAllocation& allocation);
    
    // イメージ作成・破棄
    ImageAllocation createImage(const ImageCreateInfo& createInfo);
    void destroyImage(const ImageAllocation& allocation);
    
    // メモリマッピング
    void* mapMemory(const BufferAllocation& allocation);
    void unmapMemory(const BufferAllocation& allocation);
    
    // メモリ統計情報
    void getMemoryStatistics(VmaTotalStatistics* stats) const;
    void getMemoryBudget(VmaBudget* budget) const;
    
    // デフラグメンテーション
    void defragment();
    
    // デバッグ情報
    void dumpMemoryToJson(const std::string& filePath) const;
    
private:
    const Context* m_context;
    VmaAllocator m_allocator;
    
    // VMA usage フラグ変換
    VmaMemoryUsage convertMemoryUsage(MemoryUsage usage) const;
    VmaAllocationCreateFlags getMemoryFlags(MemoryUsage usage) const;
    
    // デバッグ名設定
    void setDebugName(vk::Buffer buffer, const std::string& name) const;
    void setDebugName(vk::Image image, const std::string& name) const;
};

} // namespace rv