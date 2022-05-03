#pragma once
#include <vulkan/vulkan.hpp>

void SetImageLayout(vk::CommandBuffer commandBuffer, vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

void CopyImage(vk::CommandBuffer commandBuffer, vk::Image src, vk::Image dst, vk::ImageCopy copyRegion);

class Image
{
public:
    void Init(int width, int height, vk::Format format);
    void Init(uint32_t width, uint32_t height, vk::Format format);

    vk::Image GetImage() const { return *image; }
    vk::ImageView GetView() const { return *view; }
    vk::Sampler GetSampler() const { return *sampler; }

private:
    vk::UniqueImage image;
    vk::UniqueDeviceMemory memory;
    vk::UniqueImageView view;
    vk::UniqueSampler sampler;
};
