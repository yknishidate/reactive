#pragma once
#include "Context.hpp"

class Image
{
public:
    Image() = default;
    Image(uint32_t width, uint32_t height, vk::Format format);
    Image(const std::string& filepath);

    vk::Image GetImage() const { return *image; }
    vk::ImageView GetView() const { return *view; }
    vk::Sampler GetSampler() const { return *sampler; }

    void SetImageLayout(vk::CommandBuffer commandBuffer, vk::ImageLayout newLayout);
    static void SetImageLayout(vk::CommandBuffer commandBuffer, vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
    static void CopyImage(vk::CommandBuffer commandBuffer, vk::Image src, vk::Image dst, vk::ImageCopy copyRegion);

private:
    vk::UniqueImage image{};
    vk::UniqueDeviceMemory memory{};
    vk::UniqueImageView view{};
    vk::UniqueSampler sampler{};
    vk::ImageLayout layout = vk::ImageLayout::eUndefined;
};
