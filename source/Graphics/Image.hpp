#pragma once
#include "Context.hpp"

class Buffer;

enum class ImageUsage {
    GeneralStorage,
    ColorAttachment,
    DepthStencilAttachment,
};

class Image {
public:
    Image() = default;
    Image(uint32_t width, uint32_t height, vk::Format format, ImageUsage usage);
    Image(vk::Format format, ImageUsage usage = ImageUsage::GeneralStorage);
    Image(const std::string& filepath);
    Image(const std::vector<std::string>& filepaths);

    vk::Image getImage() const { return *image; }
    vk::ImageView getView() const { return *view; }
    vk::Sampler getSampler() const { return *sampler; }
    vk::DescriptorImageInfo getInfo() const { return {*sampler, *view, layout}; }

    void setImageLayout(vk::CommandBuffer commandBuffer, vk::ImageLayout newLayout);
    void copyToImage(vk::CommandBuffer commandBuffer, const Image& dst) const;
    void clearColor(vk::CommandBuffer commandBuffer, std::array<float, 4> color) {
        setImageLayout(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
        commandBuffer.clearColorImage(
            *image, layout, vk::ClearColorValue{color},
            vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    }

    void copyToBuffer(vk::CommandBuffer commandBuffer, Buffer& dst);
    void save(const std::string& filepath);

    static void setImageLayout(vk::CommandBuffer commandBuffer,
                               vk::Image image,
                               vk::ImageLayout newLayout,
                               vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor);

private:
    // TODO: add format
    vk::UniqueImage image{};
    vk::UniqueDeviceMemory memory{};
    vk::UniqueImageView view{};
    vk::UniqueSampler sampler{};
    vk::ImageLayout layout = vk::ImageLayout::eUndefined;
    vk::ImageAspectFlags aspect;
    uint32_t width;
    uint32_t height;
    uint32_t depth = 1;
};
