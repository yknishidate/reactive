#pragma once
#include <vulkan/vulkan.hpp>
#include "Context.hpp"

namespace rv {
class Buffer;

enum class ImageUsage {
    ColorAttachment,
    DepthAttachment,
    DepthStencilAttachment,
    Storage,
    Sampled,
};

enum class ImageLayout {
    Undefined,
    General,
    ColorAttachment,
    DepthAttachment,
    StencilAttachment,
    DepthStencilAttachment,
    ShaderReadOnly,
    TransferSrc,
    TransferDst,
    PresentSrc,
};

enum class Format {
    BGRA8Unorm,
    RGBA8Unorm,
    RGB16Sfloat,
    RGB32Sfloat,
    RGBA32Sfloat,
    D32Sfloat,
};

struct ImageCreateInfo {
    ImageUsage usage;
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    Format format;
    ImageLayout layout;
    // if mipLevels is std::numeric_limits<uint32_t>::max(), then it's set to max level
    uint32_t mipLevels = 1;
};

class Image {
public:
    Image(const Context* context, ImageCreateInfo createInfo);

    vk::Image getImage() const { return *image; }
    vk::ImageView getView() const { return *view; }
    vk::Sampler getSampler() const { return *sampler; }
    vk::DescriptorImageInfo getInfo() const { return {*sampler, *view, layout}; }
    uint32_t getMipLevels() const { return mipLevels; }
    vk::ImageAspectFlags getAspectMask() const { return aspect; }
    vk::ImageLayout getLayout() const { return layout; }

    static ImageHandle loadFromFile(const Context& context,
                                    const std::string& filepath,
                                    uint32_t mipLevels);

    // mipmap is not supported
    static ImageHandle loadFromFileHDR(const Context& context, const std::string& filepath);

    // Ensure that data is pre-filled
    // ImageLayout is implicitly shifted to ShaderReadOnlyOptimal
    void generateMipmaps();

    // void setImageLayout(vk::CommandBuffer commandBuffer, vk::ImageLayout newLayout);
    // void copyToImage(vk::CommandBuffer commandBuffer, const Image& dst) const;
    // void clearColor(vk::CommandBuffer commandBuffer, std::array<float, 4> color) {
    //     setImageLayout(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
    //     commandBuffer.clearColorImage(
    //         *image, layout, vk::ClearColorValue{color},
    //         vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    // }

    // void copyToBuffer(vk::CommandBuffer commandBuffer, Buffer& dst);
    // void save(const std::string& filepath);

    // vk::AttachmentDescription createAttachmentDesc() const;

    // vk::AttachmentReference createAttachmentRef(uint32_t attachment) const;

    static void setImageLayout(vk::CommandBuffer commandBuffer,
                               vk::Image image,
                               vk::ImageLayout oldLayout,
                               vk::ImageLayout newLayout,
                               vk::ImageAspectFlags aspect,
                               uint32_t mipLevels);

private:
    const Context* context;
    vk::UniqueImage image;
    vk::UniqueDeviceMemory memory;
    vk::UniqueImageView view;
    vk::UniqueSampler sampler;

    vk::ImageLayout layout = vk::ImageLayout::eUndefined;
    vk::Format format;
    uint32_t width;
    uint32_t height;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;
    vk::ImageAspectFlags aspect;
};
}  // namespace rv
