#pragma once
#include "Context.hpp"

class Image
{
public:
    Image() = default;
    Image(uint32_t width, uint32_t height, vk::Format format);
    Image(vk::Format format);
    Image(const std::string& filepath);
    Image(const std::vector<std::string>& filepaths);

    vk::Image GetImage() const { return *image; }
    vk::ImageView GetView() const { return *view; }
    vk::Sampler GetSampler() const { return *sampler; }
    vk::DescriptorImageInfo GetInfo() const { return { *sampler, *view, layout }; }

    void SetImageLayout(vk::CommandBuffer commandBuffer, vk::ImageLayout newLayout);
    void SetImageLayout(vk::ImageLayout newLayout);
    void CopyToImage(vk::CommandBuffer commandBuffer, const Image& dst) const;
    void CopyToImage(Image& dst);
    void CopyToBackImage() const;
    float GetAspect() const { return width / static_cast<float>(height); }

    static void SetImageLayout(vk::CommandBuffer commandBuffer, vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

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
