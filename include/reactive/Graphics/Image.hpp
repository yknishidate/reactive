#pragma once
#include <vulkan/vulkan.hpp>
#include "Context.hpp"

namespace rv {
class Buffer;

// NOTE:
// Cubemap はファイルから読み込むものと想定して
// アプリ側で作成するのは 2D or 3D のみとする
// mipLevels: UINT32_MAX の場合は画像解像度から最大ミップレベルを自動計算する
struct ImageCreateInfo {
    vk::ImageUsageFlags usage;

    vk::Extent3D extent = {1, 1, 1};

    vk::Format format;

    uint32_t mipLevels = 1;

    std::string debugName{};
};

class Image {
    friend class CommandBuffer;

public:
    Image(const Context& _context, const ImageCreateInfo& createInfo);

    Image(vk::Image image,
          vk::ImageView view,
          vk::Extent3D extent,
          vk::Format format,
          vk::ImageAspectFlags aspect)
        : image{image},
          view{view},
          viewType{vk::ImageViewType::e2D},
          extent{extent},
          format{format},
          aspect{aspect} {}

    Image(const Context* _context,
          vk::Image _image,
          vk::Format _imageFormat,
          vk::ImageLayout _imageLayout,
          vk::DeviceMemory _deviceMemory,
          vk::ImageViewType _viewType,
          uint32_t _width,
          uint32_t _height,
          uint32_t _depth,
          uint32_t _levelCount,
          uint32_t _layerCount);

    ~Image();

    void createImageView(vk::ImageViewType _viewType = vk::ImageViewType::e2D,
                         vk::ImageAspectFlags _aspect = vk::ImageAspectFlagBits::eColor) {
        viewType = _viewType;
        aspect = _aspect;

        vk::ImageSubresourceRange subresourceRange;
        subresourceRange.setAspectMask(aspect);
        subresourceRange.setBaseMipLevel(0);
        subresourceRange.setLevelCount(mipLevels);
        subresourceRange.setBaseArrayLayer(0);
        subresourceRange.setLayerCount(layerCount);

        vk::ImageViewCreateInfo viewInfo;
        viewInfo.setImage(image);
        viewInfo.setFormat(format);
        viewInfo.setViewType(viewType);
        viewInfo.setSubresourceRange(subresourceRange);

        view = context->getDevice().createImageView(viewInfo);
    }

    void createSampler(vk::Filter _filter = vk::Filter::eLinear,
                       vk::SamplerAddressMode _addressMode = vk::SamplerAddressMode::eRepeat,
                       vk::SamplerMipmapMode _mipmapMode = vk::SamplerMipmapMode::eLinear) {
        vk::SamplerCreateInfo samplerInfo;
        samplerInfo.setMagFilter(_filter);
        samplerInfo.setMinFilter(_filter);
        samplerInfo.setAnisotropyEnable(VK_FALSE);  // TODO: true
        samplerInfo.setMaxLod(0.0f);
        samplerInfo.setMinLod(0.0f);
        if (mipLevels > 1) {
            samplerInfo.setMaxLod(static_cast<float>(mipLevels));
        }
        samplerInfo.setMipmapMode(_mipmapMode);
        samplerInfo.setAddressModeU(_addressMode);
        samplerInfo.setAddressModeV(_addressMode);
        samplerInfo.setAddressModeW(_addressMode);
        samplerInfo.setCompareEnable(VK_TRUE);
        samplerInfo.setCompareOp(vk::CompareOp::eLess);
        sampler = context->getDevice().createSampler(samplerInfo);
    }

    auto getImage() const -> vk::Image { return image; }
    auto getView() const -> vk::ImageView { return view; }
    auto getSampler() const -> vk::Sampler { return sampler; }
    auto getInfo() const -> vk::DescriptorImageInfo { return {sampler, view, layout}; }
    auto getMipLevels() const -> uint32_t { return mipLevels; }
    auto getAspectMask() const -> vk::ImageAspectFlags { return aspect; }
    auto getLayout() const -> vk::ImageLayout { return layout; }
    auto getExtent() const -> vk::Extent3D { return extent; }
    auto getFormat() const -> vk::Format { return format; }
    auto getLayerCount() const -> uint32_t { return layerCount; }
    auto getViewType() const -> vk::ImageViewType { return viewType; }

    // Ensure that data is pre-filled
    // ImageLayout is implicitly shifted to ShaderReadOnlyOptimal
    void generateMipmaps(const CommandBuffer& commandBuffer);

    // TODO: refactor these
    static auto loadFromFile(const Context& context,
                             const std::filesystem::path& filepath,
                             uint32_t mipLevels = 1,
                             vk::Filter _filter = vk::Filter::eLinear,
                             vk::SamplerAddressMode _addressMode = vk::SamplerAddressMode::eRepeat)
        -> ImageHandle;

    // mipmap is not supported
    static auto loadFromFileHDR(const Context& context, const std::filesystem::path& filepath)
        -> ImageHandle;

    static auto loadFromKTX(const Context& context, const std::filesystem::path& filepath)
        -> ImageHandle;

private:
    const Context* context = nullptr;
    std::string debugName;

    vk::Image image;
    vk::DeviceMemory memory;
    vk::ImageView view;
    vk::Sampler sampler;
    vk::ImageViewType viewType;

    bool hasOwnership = false;

    vk::ImageLayout layout = vk::ImageLayout::eUndefined;
    vk::Extent3D extent;
    vk::Format format = {};

    uint32_t mipLevels = 1;
    uint32_t layerCount = 1;

    vk::ImageAspectFlags aspect;
};
}  // namespace rv
