#pragma once
#include <vulkan/vulkan.hpp>
#include "Context.hpp"
#include "MemoryManager.hpp"

namespace rv {
class Buffer;

struct ImageViewCreateInfo {
    vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor;
};

struct SamplerCreateInfo {
    vk::Filter filter = vk::Filter::eLinear;
    vk::SamplerAddressMode addressMode = vk::SamplerAddressMode::eRepeat;
    vk::SamplerMipmapMode mipmapMode = vk::SamplerMipmapMode::eLinear;
};

// NOTE:
// Cubemap はファイルから読み込むものと想定して
// アプリ側で作成するのは 2D or 3D のみとする
// mipLevels: UINT32_MAX の場合は画像解像度から最大ミップレベルを自動計算する
struct ImageCreateInfo {
    vk::ImageUsageFlags usage;
    vk::Extent3D extent = {1, 1, 1};
    vk::ImageType imageType = vk::ImageType::e2D;
    vk::Format format;
    uint32_t mipLevels = 1;
    MemoryUsage memoryUsage = MemoryUsage::GpuOnly;
    
    std::optional<ImageViewCreateInfo> viewInfo;
    std::optional<SamplerCreateInfo> samplerInfo;
    
    std::string debugName;
};

class Image {
    friend class CommandBuffer;

public:
    Image(const Context& context, const ImageCreateInfo& createInfo);

    // 外部イメージ用コンストラクタ（スワップチェーンなど）
    Image(vk::Image image,
          vk::ImageView view,
          vk::Extent3D extent,
          vk::Format format,
          vk::ImageAspectFlags aspect)
        : m_image{image},
          m_view{view},
          m_viewType{vk::ImageViewType::e2D},
          m_extent{extent},
          m_format{format},
          m_aspect{aspect},
          m_hasOwnership{false} {}

    ~Image();

    // コピー・ムーブ禁止（VMA割り当てのため）
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&&) = delete;
    Image& operator=(Image&&) = delete;

    auto getImage() const -> vk::Image { 
        return m_vmaAllocation.image ? m_vmaAllocation.image : m_image; 
    }
    auto getView() const -> vk::ImageView { return m_view; }
    auto getSampler() const -> vk::Sampler { return m_sampler; }
    auto getInfo() const -> vk::DescriptorImageInfo { return {m_sampler, m_view, m_layout}; }
    auto getMipLevels() const -> uint32_t { return m_mipLevels; }
    auto getAspectMask() const -> vk::ImageAspectFlags { return m_aspect; }
    auto getLayout() const -> vk::ImageLayout { return m_layout; }
    auto getExtent() const -> vk::Extent3D { return m_extent; }
    auto getFormat() const -> vk::Format { return m_format; }
    auto getLayerCount() const -> uint32_t { return m_layerCount; }
    auto getViewType() const -> vk::ImageViewType { return m_viewType; }
    
    // VMA関連のメソッド
    const ImageAllocation& getAllocation() const { return m_vmaAllocation; }

    // Ensure that data is pre-filled
    // ImageLayout is implicitly shifted to ShaderReadOnlyOptimal
    void generateMipmaps(const CommandBuffer& commandBuffer);

    // TODO: refactor these
    static auto loadFromFile(const Context& context,
                             const std::filesystem::path& filepath,
                             uint32_t mipLevels = 1,
                             vk::Filter filter = vk::Filter::eLinear,
                             vk::SamplerAddressMode addressMode = vk::SamplerAddressMode::eRepeat)
        -> ImageHandle;

    // mipmap is not supported
    static auto loadFromFileHDR(const Context& context, const std::filesystem::path& filepath)
        -> ImageHandle;

    static auto loadFromKTX(const Context& context, const std::filesystem::path& filepath)
        -> ImageHandle;

private:
    void createImageView(vk::ImageViewType viewType, vk::ImageAspectFlags aspect) {
        m_viewType = viewType;
        m_aspect = aspect;

        vk::ImageSubresourceRange subresourceRange;
        subresourceRange.setAspectMask(m_aspect);
        subresourceRange.setBaseMipLevel(0);
        subresourceRange.setLevelCount(m_mipLevels);
        subresourceRange.setBaseArrayLayer(0);
        subresourceRange.setLayerCount(m_layerCount);

        vk::ImageViewCreateInfo viewInfo;
        viewInfo.setImage(m_image);
        viewInfo.setFormat(m_format);
        viewInfo.setViewType(m_viewType);
        viewInfo.setSubresourceRange(subresourceRange);

        m_view = m_context->getDevice().createImageView(viewInfo);
    }

    void createSampler(vk::Filter filter,
                       vk::SamplerAddressMode addressMode,
                       vk::SamplerMipmapMode mipmapMode) {
        vk::SamplerCreateInfo samplerInfo;
        samplerInfo.setMagFilter(filter);
        samplerInfo.setMinFilter(filter);
        samplerInfo.setAnisotropyEnable(VK_FALSE);  // TODO: true
        samplerInfo.setMaxLod(0.0f);
        samplerInfo.setMinLod(0.0f);
        if (m_mipLevels > 1) {
            samplerInfo.setMaxLod(static_cast<float>(m_mipLevels));
        }
        samplerInfo.setMipmapMode(mipmapMode);
        samplerInfo.setAddressModeU(addressMode);
        samplerInfo.setAddressModeV(addressMode);
        samplerInfo.setAddressModeW(addressMode);
        samplerInfo.setCompareEnable(VK_TRUE);
        samplerInfo.setCompareOp(vk::CompareOp::eLess);
        m_sampler = m_context->getDevice().createSampler(samplerInfo);
    }

    const Context* m_context = nullptr;
    std::string m_debugName;
    bool m_hasOwnership = false;

    // VMA管理のイメージまたは外部リソース
    ImageAllocation m_vmaAllocation;
    vk::Image m_image;  // 外部リソース用
    
    vk::ImageView m_view;
    vk::Sampler m_sampler;
    vk::ImageViewType m_viewType;

    vk::ImageLayout m_layout = vk::ImageLayout::eUndefined;
    vk::Extent3D m_extent;
    vk::Format m_format = {};

    uint32_t m_mipLevels = 1;
    uint32_t m_layerCount = 1;

    vk::ImageAspectFlags m_aspect;
};
}  // namespace rv
