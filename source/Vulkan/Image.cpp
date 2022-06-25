#include "Image.hpp"
#include "Buffer.hpp"
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

    vk::UniqueImageView CreateImageView(vk::Image image, vk::Format format)
    {
        return Context::GetDevice().createImageViewUnique(
            vk::ImageViewCreateInfo()
            .setImage(image)
            .setViewType(vk::ImageViewType::e2D)
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

void Image::Init(int width, int height, vk::Format format)
{
    image = CreateImage(width, height, format);
    memory = AllocateMemory(*image);
    Context::GetDevice().bindImageMemory(*image, *memory, 0);
    view = CreateImageView(*image, format);
    sampler = CreateSampler();
    Context::OneTimeSubmit(
        [&](vk::CommandBuffer commandBuffer)
        {
            SetImageLayout(commandBuffer, *image, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);
        });
}

void Image::Init(uint32_t width, uint32_t height, vk::Format format)
{
    Init(static_cast<int>(width), static_cast<int>(height), format);
}

void Image::Init(const std::string& filepath)
{
    int width, height, channels;
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, sizeof(unsigned char) * 4);
    if (!data) {
        throw std::runtime_error("Failed to load texture: " + filepath);
    }

    Init(width, height, vk::Format::eR8G8B8A8Unorm);

    HostBuffer staging{ vk::BufferUsageFlagBits::eTransferSrc, static_cast<size_t>(width * height * 4) };
    staging.Copy(data);

    Context::OneTimeSubmit(
        [&](vk::CommandBuffer commandBuffer)
        {
            vk::BufferImageCopy region{};
            region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageExtent = vk::Extent3D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };

            SetImageLayout(commandBuffer, *image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
            commandBuffer.copyBufferToImage(staging.GetBuffer(), *image, vk::ImageLayout::eTransferDstOptimal, region);
            SetImageLayout(commandBuffer, *image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral);
        });

    stbi_image_free(data);
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

void Image::CopyImage(vk::CommandBuffer commandBuffer, vk::Image src, vk::Image dst, vk::ImageCopy copyRegion)
{
    commandBuffer.copyImage(src, vk::ImageLayout::eTransferSrcOptimal,
                            dst, vk::ImageLayout::eTransferDstOptimal, copyRegion);
}
