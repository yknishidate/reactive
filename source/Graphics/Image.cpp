#include "Image.hpp"
#include "Buffer.hpp"
#include "Window/Window.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

Image::Image(uint32_t width, uint32_t height, vk::Format format, ImageUsage _usage)
    : width{width}, height{height}, format{format}, type{vk::ImageType::e2D} {
    vk::ImageLayout newLayout;
    switch (_usage) {
        case ImageUsage::GeneralStorage:
            usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled |
                    vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
            newLayout = vk::ImageLayout::eGeneral;
            aspect = vk::ImageAspectFlagBits::eColor;
            break;
        case ImageUsage::ColorAttachment:
            usage = vk::ImageUsageFlagBits::eColorAttachment |
                    vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
            newLayout = vk::ImageLayout::eColorAttachmentOptimal;
            aspect = vk::ImageAspectFlagBits::eColor;
            break;
        case ImageUsage::DepthStencilAttachment:
            usage = vk::ImageUsageFlagBits::eDepthStencilAttachment |
                    vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
            newLayout = vk::ImageLayout::eDepthAttachmentOptimal;
            aspect = vk::ImageAspectFlagBits::eDepth;
            break;
        default:
            assert(false);
    }

    createImage();
    allocateMemory();
    bindImageMemory();
    createImageView();
    createSampler();

    Context::oneTimeSubmit(
        [&](vk::CommandBuffer commandBuffer) { setImageLayout(commandBuffer, newLayout); });
}

Image::Image(const std::string& filepath) {
    int w;
    int h;
    int channels;
    unsigned char* data = stbi_load(filepath.c_str(), &w, &h, &channels, sizeof(unsigned char) * 4);
    if (!data) {
        throw std::runtime_error("Failed to load texture: " + filepath);
    }

    width = w;
    height = h;
    format = vk::Format::eR8G8B8A8Unorm;
    usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eTransferDst;
    aspect = vk::ImageAspectFlagBits::eColor;
    createImage();
    allocateMemory();
    bindImageMemory();
    createImageView();
    createSampler();

    HostBuffer staging{BufferUsage::Staging, static_cast<size_t>(width * height * 4)};
    staging.copy(data);
    Context::oneTimeSubmit([&](vk::CommandBuffer commandBuffer) {
        vk::BufferImageCopy region{};
        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = vk::Extent3D{width, height, 1};

        setImageLayout(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
        commandBuffer.copyBufferToImage(staging.getBuffer(), *image,
                                        vk::ImageLayout::eTransferDstOptimal, region);
        setImageLayout(commandBuffer, vk::ImageLayout::eGeneral);
    });

    stbi_image_free(data);
}

Image::Image(const std::vector<std::string>& filepaths) {
    // TODO: Don't repeat yourself
    int width;
    int height;
    int channels;
    unsigned char* data =
        stbi_load(filepaths[0].c_str(), &width, &height, &channels, sizeof(unsigned char) * 4);
    if (!data) {
        throw std::runtime_error("Failed to load texture: " + filepaths[0]);
    }
    stbi_image_free(data);
    this->width = width;
    this->height = height;
    this->depth = filepaths.size();
    this->type = vk::ImageType::e3D;

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

    format = vk::Format::eR8G8B8A8Unorm;
    usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eTransferDst;
    aspect = vk::ImageAspectFlagBits::eColor;
    createImage();
    allocateMemory();
    bindImageMemory();
    createImageView();
    createSampler();

    HostBuffer staging{BufferUsage::Staging, static_cast<size_t>(width * height * depth * 4)};
    staging.copy(allData.data());
    Context::oneTimeSubmit([&](vk::CommandBuffer commandBuffer) {
        vk::BufferImageCopy region{};
        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageExtent =
            vk::Extent3D{static_cast<uint32_t>(width), static_cast<uint32_t>(height), depth};
        setImageLayout(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
        commandBuffer.copyBufferToImage(staging.getBuffer(), *image,
                                        vk::ImageLayout::eTransferDstOptimal, region);
        setImageLayout(commandBuffer, vk::ImageLayout::eGeneral);
    });
}

void Image::setImageLayout(vk::CommandBuffer commandBuffer, vk::ImageLayout newLayout) {
    setImageLayout(commandBuffer, *image, newLayout, aspect);
    layout = newLayout;
}

void Image::copyToImage(vk::CommandBuffer commandBuffer, const Image& dst) const {
    vk::ImageCopy copyRegion;
    copyRegion.setSrcSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});
    copyRegion.setDstSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});
    copyRegion.setExtent({width, height, 1});

    vk::Image srcImage = *image;
    vk::Image dstImage = dst.getImage();

    commandBuffer.copyImage(srcImage, vk::ImageLayout::eTransferSrcOptimal, dstImage,
                            vk::ImageLayout::eTransferDstOptimal, copyRegion);
}

void Image::copyToBuffer(vk::CommandBuffer commandBuffer, Buffer& dst) {
    vk::ImageSubresourceLayers subresource;
    subresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
    subresource.setBaseArrayLayer(0);
    subresource.setLayerCount(1);
    subresource.setMipLevel(0);

    vk::BufferImageCopy copyInfo;
    copyInfo.setBufferOffset(0);
    copyInfo.setBufferRowLength(width);
    copyInfo.setBufferImageHeight(height);
    copyInfo.setImageExtent({width, height, 1});
    copyInfo.setImageSubresource(subresource);

    auto oldLayout = layout;
    setImageLayout(commandBuffer, vk::ImageLayout::eTransferSrcOptimal);
    commandBuffer.copyImageToBuffer(*image, vk::ImageLayout::eTransferSrcOptimal, dst.getBuffer(),
                                    copyInfo);
    setImageLayout(commandBuffer, oldLayout);
}

void Image::save(const std::string& filepath) {
    // TODO: remove Window
    static vk::DeviceSize size = Window::getWidth() * Window::getWidth() * 4;
    static HostBuffer buffer{BufferUsage::Staging, size};
    static uint8_t* pixels = static_cast<uint8_t*>(buffer.map());

    Context::oneTimeSubmit(
        [&](vk::CommandBuffer commandBuffer) { copyToBuffer(commandBuffer, buffer); });

    std::vector<uint8_t> output(width * height * 3);
    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
            int index = h * width + w;
            output[index * 3 + 0] = pixels[index * 4 + 2];
            output[index * 3 + 1] = pixels[index * 4 + 1];
            output[index * 3 + 2] = pixels[index * 4 + 0];
        }
    }

    stbi_write_png(filepath.c_str(), width, height, 3, output.data(), width * 3);
}

void Image::setImageLayout(vk::CommandBuffer commandBuffer,
                           vk::Image image,
                           vk::ImageLayout newLayout,
                           vk::ImageAspectFlags aspect) {
    vk::PipelineStageFlags srcStageMask = vk::PipelineStageFlagBits::eAllCommands;
    vk::PipelineStageFlags dstStageMask = vk::PipelineStageFlagBits::eAllCommands;

    vk::ImageMemoryBarrier barrier{};
    barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setImage(image);
    barrier.setOldLayout(vk::ImageLayout::eUndefined);
    barrier.setNewLayout(newLayout);
    barrier.setSubresourceRange({aspect, 0, 1, 0, 1});
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

void Image::createImage() {
    vk::ImageCreateInfo imageInfo;
    imageInfo.setImageType(type);
    imageInfo.setFormat(format);
    imageInfo.setExtent({width, height, depth});
    imageInfo.setMipLevels(1);
    imageInfo.setArrayLayers(1);
    imageInfo.setSamples(vk::SampleCountFlagBits::e1);
    imageInfo.setUsage(usage);
    image = Context::getDevice().createImageUnique(imageInfo);
}

void Image::allocateMemory() {
    vk::MemoryRequirements requirements = Context::getDevice().getImageMemoryRequirements(*image);
    uint32_t memoryTypeIndex =
        Context::findMemoryTypeIndex(requirements, vk::MemoryPropertyFlagBits::eDeviceLocal);
    vk::MemoryAllocateInfo memoryInfo;
    memoryInfo.setAllocationSize(requirements.size);
    memoryInfo.setMemoryTypeIndex(memoryTypeIndex);
    memory = Context::getDevice().allocateMemoryUnique(memoryInfo);
}

void Image::createImageView() {
    vk::ImageViewCreateInfo viewInfo;
    viewInfo.setImage(*image);
    viewInfo.setFormat(format);
    viewInfo.setSubresourceRange({aspect, 0, 1, 0, 1});
    if (type == vk::ImageType::e2D) {
        viewInfo.setViewType(vk::ImageViewType::e2D);
    } else if (type == vk::ImageType::e3D) {
        viewInfo.setViewType(vk::ImageViewType::e3D);
    } else {
        assert(false);
    }
    view = Context::getDevice().createImageViewUnique(viewInfo);
}

void Image::bindImageMemory() {
    Context::getDevice().bindImageMemory(*image, *memory, 0);
}

void Image::createSampler() {
    sampler = Context::getDevice().createSamplerUnique(
        vk::SamplerCreateInfo()
            .setMagFilter(vk::Filter::eLinear)
            .setMinFilter(vk::Filter::eLinear)
            .setAnisotropyEnable(VK_FALSE)
            .setMaxLod(0.0f)
            .setMinLod(0.0f)
            .setMipmapMode(vk::SamplerMipmapMode::eLinear)
            .setAddressModeU(vk::SamplerAddressMode::eRepeat)
            .setAddressModeV(vk::SamplerAddressMode::eRepeat)
            .setAddressModeW(vk::SamplerAddressMode::eRepeat));
}
