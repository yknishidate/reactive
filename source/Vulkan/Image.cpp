#include "Image.hpp"
#include "Buffer.hpp"
#include "Window/Window.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace
{
    vk::UniqueImage CreateImage(uint32_t width, uint32_t height, vk::Format format)
    {
        using Usage = vk::ImageUsageFlagBits;
        return Context::GetDevice().createImageUnique(
            vk::ImageCreateInfo()
            .setImageType(vk::ImageType::e2D)
            .setFormat(format)
            .setExtent({ width, height, 1 })
            .setMipLevels(1)
            .setArrayLayers(1)
            .setUsage(Usage::eStorage | Usage::eSampled | Usage::eTransferSrc | Usage::eTransferDst)
        );
    }

    vk::UniqueImage CreateImage(uint32_t width, uint32_t height, uint32_t depth, vk::Format format)
    {
        using Usage = vk::ImageUsageFlagBits;
        return Context::GetDevice().createImageUnique(
            vk::ImageCreateInfo()
            .setImageType(vk::ImageType::e3D)
            .setFormat(format)
            .setExtent({ width, height, depth })
            .setMipLevels(1)
            .setArrayLayers(1)
            .setUsage(Usage::eStorage | Usage::eSampled | Usage::eTransferSrc | Usage::eTransferDst)
        );
    }

    vk::UniqueDeviceMemory AllocateMemory(vk::Image image)
    {
        vk::MemoryRequirements requirements = Context::GetDevice().getImageMemoryRequirements(image);
        uint32_t memoryTypeIndex = Context::FindMemoryTypeIndex(requirements, vk::MemoryPropertyFlagBits::eDeviceLocal);
        return Context::GetDevice().allocateMemoryUnique(
            vk::MemoryAllocateInfo()
            .setAllocationSize(requirements.size)
            .setMemoryTypeIndex(memoryTypeIndex)
        );
    }

    vk::UniqueImageView CreateImageView(vk::Image image, vk::Format format, vk::ImageViewType type = vk::ImageViewType::e2D)
    {
        return Context::GetDevice().createImageViewUnique(
            vk::ImageViewCreateInfo()
            .setImage(image)
            .setViewType(type)
            .setFormat(format)
            .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 })
        );
    }

    vk::UniqueSampler CreateSampler()
    {
        return Context::GetDevice().createSamplerUnique(
            vk::SamplerCreateInfo()
            .setMagFilter(vk::Filter::eLinear)
            .setMinFilter(vk::Filter::eLinear)
            .setAnisotropyEnable(VK_FALSE)
            .setMaxLod(0.0f)
            .setMinLod(0.0f)
            .setMipmapMode(vk::SamplerMipmapMode::eLinear)
            .setAddressModeU(vk::SamplerAddressMode::eRepeat)
            .setAddressModeV(vk::SamplerAddressMode::eRepeat)
            .setAddressModeW(vk::SamplerAddressMode::eRepeat)
        );
    }
}

Image::Image(uint32_t width, uint32_t height, vk::Format format)
    : width{ width }
    , height{ height }
{
    image = CreateImage(width, height, format);
    memory = AllocateMemory(*image);
    Context::GetDevice().bindImageMemory(*image, *memory, 0);

    view = CreateImageView(*image, format);
    sampler = CreateSampler();

    Context::OneTimeSubmit(
        [&](vk::CommandBuffer commandBuffer)
        {
            SetImageLayout(commandBuffer, vk::ImageLayout::eGeneral);
        });
}

Image::Image(vk::Format format)
    : Image(Window::GetWidth(), Window::GetHeight(), format)
{
}

Image::Image(const std::string& filepath)
{
    int width;
    int height;
    int channels;
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, sizeof(unsigned char) * 4);
    if (!data) {
        throw std::runtime_error("Failed to load texture: " + filepath);
    }

    this->width = width;
    this->height = height;
    image = CreateImage(width, height, vk::Format::eR8G8B8A8Unorm);
    memory = AllocateMemory(*image);
    Context::GetDevice().bindImageMemory(*image, *memory, 0);

    view = CreateImageView(*image, vk::Format::eR8G8B8A8Unorm);
    sampler = CreateSampler();

    StagingBuffer staging{ static_cast<size_t>(width * height * 4), data };
    Context::OneTimeSubmit(
        [&](vk::CommandBuffer commandBuffer)
        {
            vk::BufferImageCopy region{};
            region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageExtent = vk::Extent3D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };

            SetImageLayout(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
            commandBuffer.copyBufferToImage(staging.GetBuffer(), *image, vk::ImageLayout::eTransferDstOptimal, region);
            SetImageLayout(commandBuffer, vk::ImageLayout::eGeneral);
        });

    stbi_image_free(data);
}

Image::Image(const std::vector<std::string>& filepaths)
{
    // TODO: create stb wrapper
    int width;
    int height;
    int channels;
    unsigned char* data = stbi_load(filepaths[0].c_str(), &width, &height, &channels, sizeof(unsigned char) * 4);
    if (!data) {
        throw std::runtime_error("Failed to load texture: " + filepaths[0]);
    }
    stbi_image_free(data);
    this->width = width;
    this->height = height;
    this->depth = filepaths.size();

    std::vector<uint8_t> allData(width * height * depth * 4);
    size_t offset = 0;
    for (const auto& filepath : filepaths) {
        spdlog::info("Load image: {}", filepath);
        data = stbi_load(filepath.c_str(), &width, &height, &channels, sizeof(unsigned char) * 4);
        auto size = width * height * 4;
        std::copy_n(data, size, allData.begin() + offset);
        offset += size;
        stbi_image_free(data);
    }

    image = CreateImage(width, height, depth, vk::Format::eR8G8B8A8Unorm);
    memory = AllocateMemory(*image);
    Context::GetDevice().bindImageMemory(*image, *memory, 0);

    view = CreateImageView(*image, vk::Format::eR8G8B8A8Unorm, vk::ImageViewType::e3D);
    sampler = CreateSampler();

    StagingBuffer staging{ static_cast<size_t>(width * height * depth * 4), allData.data() };
    Context::OneTimeSubmit(
        [&](vk::CommandBuffer commandBuffer)
        {
            vk::BufferImageCopy region{};
            region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageExtent = vk::Extent3D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height), depth };
            SetImageLayout(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
            commandBuffer.copyBufferToImage(staging.GetBuffer(), *image, vk::ImageLayout::eTransferDstOptimal, region);
            SetImageLayout(commandBuffer, vk::ImageLayout::eGeneral);
        });
}

void Image::SetImageLayout(vk::CommandBuffer commandBuffer, vk::ImageLayout newLayout)
{
    SetImageLayout(commandBuffer, *image, layout, newLayout);
    layout = newLayout;
}

void Image::SetImageLayout(vk::ImageLayout newLayout)
{
    SetImageLayout(Context::GetCurrentCommandBuffer(), newLayout);
}

void Image::CopyToImage(vk::CommandBuffer commandBuffer, const Image& dst) const
{
    vk::ImageCopy copyRegion;
    copyRegion.setSrcSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });
    copyRegion.setDstSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });
    copyRegion.setExtent({ width, height, 1 });

    vk::Image srcImage = *image;
    vk::Image dstImage = dst.GetImage();

    commandBuffer.copyImage(srcImage, vk::ImageLayout::eTransferSrcOptimal,
                            dstImage, vk::ImageLayout::eTransferDstOptimal, copyRegion);
}

void Image::CopyToImage(Image& dst)
{
    vk::ImageLayout srcOldLayout = layout;
    vk::ImageLayout dstOldLayout = dst.layout;
    SetImageLayout(vk::ImageLayout::eTransferSrcOptimal);
    dst.SetImageLayout(vk::ImageLayout::eTransferDstOptimal);

    CopyToImage(Context::GetCurrentCommandBuffer(), dst);

    SetImageLayout(srcOldLayout);
    dst.SetImageLayout(dstOldLayout);
}

void Image::CopyToBackImage() const
{
    Context::CopyToBackImage(Context::GetCurrentCommandBuffer(), *this);
}

void Image::SetImageLayout(vk::CommandBuffer commandBuffer, vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    vk::PipelineStageFlags srcStageMask = vk::PipelineStageFlagBits::eAllCommands;
    vk::PipelineStageFlags dstStageMask = vk::PipelineStageFlagBits::eAllCommands;

    vk::ImageMemoryBarrier barrier{};
    barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setImage(image);
    barrier.setOldLayout(oldLayout);
    barrier.setNewLayout(newLayout);
    barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

    switch (oldLayout) {
    case vk::ImageLayout::eColorAttachmentOptimal:
        barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        break;
    case vk::ImageLayout::eTransferSrcOptimal:
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        break;
    case vk::ImageLayout::eTransferDstOptimal:
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        break;
    default:
        break;
    }
    switch (newLayout) {
    case vk::ImageLayout::eTransferDstOptimal:
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        break;
    case vk::ImageLayout::eTransferSrcOptimal:
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        break;
    case vk::ImageLayout::eColorAttachmentOptimal:
        barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        break;
    default:
        break;
    }
    commandBuffer.pipelineBarrier(srcStageMask, dstStageMask, {}, {}, {}, barrier);
}
