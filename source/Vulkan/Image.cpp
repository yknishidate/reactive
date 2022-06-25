#include "Image.hpp"
#include "Buffer.hpp"
#include <stb_image.h>

namespace
{
    vk::UniqueImage CreateImage(uint32_t width, uint32_t height, vk::Format format)
    {
        vk::ImageCreateInfo imageCreateInfo{};
        imageCreateInfo.setImageType(vk::ImageType::e2D);
        imageCreateInfo.setFormat(format);
        imageCreateInfo.setExtent({ width, height, 1 });
        imageCreateInfo.setMipLevels(1);
        imageCreateInfo.setArrayLayers(1);
        imageCreateInfo.setUsage(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled |
                                 vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst);
        return Context::GetDevice().createImageUnique(imageCreateInfo);
    }

    vk::UniqueDeviceMemory AllocateMemory(vk::Image image)
    {
        vk::MemoryRequirements requirements = Context::GetDevice().getImageMemoryRequirements(image);
        uint32_t memoryTypeIndex = Context::FindMemoryTypeIndex(requirements, vk::MemoryPropertyFlagBits::eDeviceLocal);
        vk::MemoryAllocateInfo memoryAllocateInfo{};
        memoryAllocateInfo.setAllocationSize(requirements.size);
        memoryAllocateInfo.setMemoryTypeIndex(memoryTypeIndex);
        return Context::GetDevice().allocateMemoryUnique(memoryAllocateInfo);
    }

    vk::UniqueImageView CreateImageView(vk::Image image, vk::Format format)
    {
        vk::ImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.setImage(image);
        imageViewCreateInfo.setViewType(vk::ImageViewType::e2D);
        imageViewCreateInfo.setFormat(format);
        imageViewCreateInfo.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
        return Context::GetDevice().createImageViewUnique(imageViewCreateInfo);
    }

    vk::UniqueSampler CreateSampler()
    {
        vk::SamplerCreateInfo createInfo{};
        createInfo.setMagFilter(vk::Filter::eLinear);
        createInfo.setMinFilter(vk::Filter::eLinear);
        createInfo.setAnisotropyEnable(VK_FALSE);
        createInfo.setMaxLod(0.0f);
        createInfo.setMinLod(0.0f);
        createInfo.setMipmapMode(vk::SamplerMipmapMode::eLinear);
        createInfo.setAddressModeU(vk::SamplerAddressMode::eRepeat);
        createInfo.setAddressModeV(vk::SamplerAddressMode::eRepeat);
        createInfo.setAddressModeW(vk::SamplerAddressMode::eRepeat);
        return Context::GetDevice().createSamplerUnique(createInfo);
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
