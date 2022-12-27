#pragma once
#include "Context.hpp"

class Buffer;

class Image
{
public:
    Image() = default;
    Image(uint32_t width, uint32_t height, vk::Format format);
    Image(vk::Format format);
    Image(const std::string& filepath);
    Image(const std::vector<std::string>& filepaths);

    vk::Image getImage() const { return *image; }
    vk::ImageView getView() const { return *view; }
    vk::Sampler getSampler() const { return *sampler; }
    vk::DescriptorImageInfo getInfo() const { return { *sampler, *view, layout }; }

    void setImageLayout(vk::CommandBuffer commandBuffer, vk::ImageLayout newLayout);
    void copyToImage(vk::CommandBuffer commandBuffer, const Image& dst) const;

    void copyToBuffer(vk::CommandBuffer commandBuffer, Buffer& dst);
    void save(const std::string& filepath);

    static void setImageLayout(vk::CommandBuffer commandBuffer, vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

private:
    vk::UniqueImage image{};
    vk::UniqueDeviceMemory memory{};
    vk::UniqueImageView view{};
    vk::UniqueSampler sampler{};
    vk::ImageLayout layout = vk::ImageLayout::eUndefined;
    uint32_t width;
    uint32_t height;
    uint32_t depth = 1;
};
