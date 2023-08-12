#include "Graphics/Image.hpp"
#include "Graphics/Buffer.hpp"
#include "common.hpp"

namespace {
uint32_t calculateMipLevels(uint32_t width, uint32_t height) {
    return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}
vk::ImageUsageFlags getImageUsage(rv::ImageUsage usage) {
    switch (usage) {
        case rv::ImageUsage::ColorAttachment:
            return vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc |
                   vk::ImageUsageFlagBits::eTransferDst;
        case rv::ImageUsage::DepthAttachment:
            return vk::ImageUsageFlagBits::eDepthStencilAttachment;
        case rv::ImageUsage::DepthStencilAttachment:
            return vk::ImageUsageFlagBits::eDepthStencilAttachment;
        case rv::ImageUsage::Sampled:
            return vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst |
                   vk::ImageUsageFlagBits::eTransferSrc;
        case rv::ImageUsage::Storage:
            return vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst |
                   vk::ImageUsageFlagBits::eTransferSrc;
    }
}

vk::ImageAspectFlags getImageAspect(rv::ImageUsage usage) {
    switch (usage) {
        case rv::ImageUsage::ColorAttachment:
            return vk::ImageAspectFlagBits::eColor;
        case rv::ImageUsage::DepthAttachment:
            return vk::ImageAspectFlagBits::eDepth;
        case rv::ImageUsage::DepthStencilAttachment:
            return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        case rv::ImageUsage::Sampled:
            return vk::ImageAspectFlagBits::eColor;
        case rv::ImageUsage::Storage:
            return vk::ImageAspectFlagBits::eColor;
    }
}
}  // namespace

namespace rv {
Image::Image(const Context* context, ImageCreateInfo createInfo)
    : context{context},
      width{createInfo.width},
      height{createInfo.height},
      depth{createInfo.depth},
      layout{static_cast<vk::ImageLayout>(createInfo.layout)},
      format{static_cast<vk::Format>(createInfo.format)},
      mipLevels{createInfo.mipLevels},
      aspect{getImageAspect(createInfo.usage)} {
    vk::ImageType type = createInfo.depth == 1 ? vk::ImageType::e2D : vk::ImageType::e3D;

    // Compute mipmap level
    if (createInfo.mipLevels == std::numeric_limits<uint32_t>::max()) {
        mipLevels = calculateMipLevels(width, height);
        spdlog::info("MipLevels: {}", mipLevels);
    }

    // NOTE: initialLayout must be Undefined or PreInitialized
    // NOTE: queueFamily is ignored if sharingMode is not concurrent
    vk::ImageCreateInfo imageInfo;
    imageInfo.setImageType(type);
    imageInfo.setFormat(format);
    imageInfo.setExtent({width, height, depth});
    imageInfo.setMipLevels(mipLevels);
    imageInfo.setArrayLayers(1);
    imageInfo.setSamples(vk::SampleCountFlagBits::e1);
    imageInfo.setUsage(getImageUsage(createInfo.usage));
    image = context->getDevice().createImageUnique(imageInfo);

    vk::MemoryRequirements requirements = context->getDevice().getImageMemoryRequirements(*image);
    uint32_t memoryTypeIndex =
        context->findMemoryTypeIndex(requirements, vk::MemoryPropertyFlagBits::eDeviceLocal);
    vk::MemoryAllocateInfo memoryInfo;
    memoryInfo.setAllocationSize(requirements.size);
    memoryInfo.setMemoryTypeIndex(memoryTypeIndex);
    memory = context->getDevice().allocateMemoryUnique(memoryInfo);

    context->getDevice().bindImageMemory(*image, *memory, 0);

    vk::ImageSubresourceRange subresourceRange;
    subresourceRange.setAspectMask(aspect);
    subresourceRange.setBaseMipLevel(0);
    subresourceRange.setLevelCount(mipLevels);
    subresourceRange.setBaseArrayLayer(0);
    subresourceRange.setLayerCount(1);

    vk::ImageViewCreateInfo viewInfo;
    viewInfo.setImage(*image);
    viewInfo.setFormat(format);
    viewInfo.setSubresourceRange(subresourceRange);
    if (type == vk::ImageType::e2D) {
        viewInfo.setViewType(vk::ImageViewType::e2D);
    } else if (type == vk::ImageType::e3D) {
        viewInfo.setViewType(vk::ImageViewType::e3D);
    } else {
        assert(false);
    }

    view = context->getDevice().createImageViewUnique(viewInfo);

    vk::SamplerCreateInfo samplerInfo;
    samplerInfo.setMagFilter(vk::Filter::eLinear);
    samplerInfo.setMinFilter(vk::Filter::eLinear);
    samplerInfo.setAnisotropyEnable(VK_FALSE);
    samplerInfo.setMaxLod(0.0f);
    samplerInfo.setMinLod(0.0f);
    if (mipLevels > 1) {
        samplerInfo.setMinLod(static_cast<float>(mipLevels));
    }
    samplerInfo.setMipmapMode(vk::SamplerMipmapMode::eLinear);
    samplerInfo.setAddressModeU(vk::SamplerAddressMode::eRepeat);
    samplerInfo.setAddressModeV(vk::SamplerAddressMode::eRepeat);
    samplerInfo.setAddressModeW(vk::SamplerAddressMode::eRepeat);
    sampler = context->getDevice().createSamplerUnique(samplerInfo);

    context->oneTimeSubmit([&](vk::CommandBuffer commandBuffer) {
        setImageLayout(commandBuffer, *image, vk::ImageLayout::eUndefined, layout, aspect,
                       mipLevels);
    });
}

Image Image::loadFromFile(const Context& context, const std::string& filepath, uint32_t mipLevels) {
    int width;
    int height;
    int comp = 4;
    unsigned char* pixels = stbi_load(filepath.c_str(), &width, &height, nullptr, comp);
    if (!pixels) {
        throw std::runtime_error("Failed to load image: " + filepath);
    }

    Image image = context.createImage({
        .usage = ImageUsage::Sampled,
        .layout = ImageLayout::TransferDst,
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .depth = 1,
        .format = Format::RGBA8Unorm,
        .mipLevels = mipLevels,
    });

    // Copy to image
    Buffer stagingBuffer = context.createBuffer({
        .usage = BufferUsage::Staging,
        .memory = MemoryUsage::Host,
        .size = width * height * comp * sizeof(unsigned char*),
        .data = reinterpret_cast<void*>(pixels),
    });

    context.oneTimeSubmit([&](vk::CommandBuffer commandBuffer) {
        vk::ImageSubresourceLayers subresourceLayers;
        subresourceLayers.setAspectMask(vk::ImageAspectFlagBits::eColor);
        subresourceLayers.setMipLevel(0);
        subresourceLayers.setBaseArrayLayer(0);
        subresourceLayers.setLayerCount(1);

        vk::Extent3D extent;
        extent.setWidth(static_cast<uint32_t>(width));
        extent.setHeight(static_cast<uint32_t>(height));
        extent.setDepth(1);

        vk::BufferImageCopy region;
        region.imageSubresource = subresourceLayers;
        region.imageExtent = extent;

        commandBuffer.copyBufferToImage(stagingBuffer.getBuffer(), image.getImage(),
                                        vk::ImageLayout::eTransferDstOptimal, region);

        vk::ImageLayout newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        if (mipLevels > 1) {
            newLayout = vk::ImageLayout::eTransferSrcOptimal;
        }
        Image::setImageLayout(commandBuffer, image.getImage(), vk::ImageLayout::eTransferDstOptimal,
                              newLayout, vk::ImageAspectFlagBits::eColor, image.getMipLevels());
    });

    if (mipLevels > 1) {
        image.generateMipmaps();
    }

    stbi_image_free(pixels);

    return image;
}

Image Image::loadFromFileHDR(const Context& context, const std::string& filepath) {
    int width;
    int height;
    int comp = 4;
    float* pixels = stbi_loadf(filepath.c_str(), &width, &height, nullptr, comp);
    if (!pixels) {
        throw std::runtime_error("Failed to load image: " + filepath);
    }

    Image image = context.createImage({
        .usage = ImageUsage::Sampled,
        .layout = ImageLayout::TransferDst,
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .depth = 1,
        .format = Format::RGBA32Sfloat,
    });

    // Copy to image
    Buffer stagingBuffer = context.createBuffer({
        .usage = BufferUsage::Staging,
        .memory = MemoryUsage::Host,
        .size = width * height * comp * sizeof(float),
        .data = reinterpret_cast<void*>(pixels),
    });

    context.oneTimeSubmit([&](vk::CommandBuffer commandBuffer) {
        vk::ImageSubresourceLayers subresourceLayers;
        subresourceLayers.setAspectMask(vk::ImageAspectFlagBits::eColor);
        subresourceLayers.setMipLevel(0);
        subresourceLayers.setBaseArrayLayer(0);
        subresourceLayers.setLayerCount(1);

        vk::Extent3D extent;
        extent.setWidth(static_cast<uint32_t>(width));
        extent.setHeight(static_cast<uint32_t>(height));
        extent.setDepth(1);

        vk::BufferImageCopy region;
        region.imageSubresource = subresourceLayers;
        region.imageExtent = extent;

        commandBuffer.copyBufferToImage(stagingBuffer.getBuffer(), image.getImage(),
                                        vk::ImageLayout::eTransferDstOptimal, region);
        Image::setImageLayout(commandBuffer, image.getImage(), vk::ImageLayout::eTransferDstOptimal,
                              vk::ImageLayout::eShaderReadOnlyOptimal,
                              vk::ImageAspectFlagBits::eColor, image.getMipLevels());
        image.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
    });

    stbi_image_free(pixels);

    return image;
}

void Image::generateMipmaps() {
    RV_ASSERT(mipLevels > 1, "mipLevels is not set greater than 1 when the image is created.");

    // Check if image format supports linear blitting
    vk::Filter filter = vk::Filter::eLinear;
    vk::FormatProperties formatProperties =
        context->getPhysicalDevice().getFormatProperties(format);
    if (!(formatProperties.optimalTilingFeatures &
          vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        spdlog::warn("This format does not support linear blitting: {}", vk::to_string(format));
        filter = vk::Filter::eNearest;
    }
    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitSrc)) {
        RV_ASSERT(false, "This format does not suppoprt blitting: {}", vk::to_string(format));
    }
    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst)) {
        RV_ASSERT(false, "This format does not suppoprt blitting: {}", vk::to_string(format));
    }

    // TODO: support 3D
    context->oneTimeSubmit([&](vk::CommandBuffer commandBuffer) {
        vk::ImageMemoryBarrier barrier{};
        barrier.image = *image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = width;
        int32_t mipHeight = height;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

            commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                          vk::PipelineStageFlagBits::eTransfer, {}, nullptr,
                                          nullptr, barrier);

            vk::ImageBlit blit{};
            blit.srcOffsets[0] = vk::Offset3D{0, 0, 0};
            blit.srcOffsets[1] = vk::Offset3D{mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
            blit.dstOffsets[1] =
                vk::Offset3D{mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
            blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            commandBuffer.blitImage(*image, vk::ImageLayout::eTransferSrcOptimal, *image,
                                    vk::ImageLayout::eTransferDstOptimal, blit, filter);

            barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                          vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr,
                                          nullptr, barrier);

            if (mipWidth > 1)
                mipWidth /= 2;
            if (mipHeight > 1)
                mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                      vk::PipelineStageFlagBits::eAllCommands, {}, nullptr, nullptr,
                                      barrier);
    });
}

// #define STB_IMAGE_IMPLEMENTATION
// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include <stb_image.h>
// #include <stb_image_write.h>
//
// Image::Image(uint32_t width, uint32_t height, vk::Format format, ImageUsage _usage)
//     : format{format}, type{vk::ImageType::e2D}, width{width}, height{height} {
//     vk::ImageLayout newLayout;
//     switch (_usage) {
//         case ImageUsage::GeneralStorage:
//             usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled |
//                     vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
//             newLayout = vk::ImageLayout::eGeneral;
//             aspect = vk::ImageAspectFlagBits::eColor;
//             break;
//         case ImageUsage::ColorAttachment:
//             usage = vk::ImageUsageFlagBits::eColorAttachment |
//                     vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
//             newLayout = vk::ImageLayout::eColorAttachmentOptimal;
//             aspect = vk::ImageAspectFlagBits::eColor;
//             break;
//         case ImageUsage::DepthStencilAttachment:
//             usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
//             newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
//             aspect = vk::ImageAspectFlagBits::eDepth;
//             break;
//         default:
//             assert(false);
//     }
//
//     createImage();
//     allocateMemory();
//     bindImageMemory();
//     createImageView();
//     createSampler();
//
//     Context::oneTimeSubmit(
//         [&](vk::CommandBuffer commandBuffer) { setImageLayout(commandBuffer, newLayout); });
// }
//
// Image::Image(const std::string& filepath) {
//     unsigned char* data = loadFile(filepath);
//
//     format = vk::Format::eR8G8B8A8Unorm;
//     usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled |
//             vk::ImageUsageFlagBits::eTransferDst;
//     aspect = vk::ImageAspectFlagBits::eColor;
//     createImage();
//     allocateMemory();
//     bindImageMemory();
//     createImageView();
//     createSampler();
//
//     Buffer staging{BufferUsage::Staging, static_cast<size_t>(width * height * depth * 4)};
//     staging.copy(data);
//     Context::oneTimeSubmit([&](vk::CommandBuffer commandBuffer) {
//         vk::BufferImageCopy region;
//         region.setImageExtent({width, height, depth});
//         region.setImageSubresource({aspect, 0, 0, 1});
//         region.imageSubresource.aspectMask = aspect;
//
//         setImageLayout(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
//         commandBuffer.copyBufferToImage(staging.getBuffer(), *image,
//                                         vk::ImageLayout::eTransferDstOptimal, region);
//         setImageLayout(commandBuffer, vk::ImageLayout::eGeneral);
//     });
//
//     stbi_image_free(data);
// }
//
// Image::Image(uint32_t width, uint32_t height, Buffer& buffer, size_t offset)
//     : width{width}, height{height} {
//     format = vk::Format::eR8G8B8A8Unorm;
//     usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled |
//             vk::ImageUsageFlagBits::eTransferDst;
//     aspect = vk::ImageAspectFlagBits::eColor;
//     createImage();
//     allocateMemory();
//     bindImageMemory();
//     createImageView();
//     createSampler();
//
//     Context::oneTimeSubmit([&](vk::CommandBuffer commandBuffer) {
//         vk::BufferImageCopy region;
//         region.setBufferOffset(offset);
//         region.setImageExtent({width, height, 1});
//         region.setImageSubresource({aspect, 0, 0, 1});
//
//         setImageLayout(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
//         commandBuffer.copyBufferToImage(buffer.getBuffer(), *image,
//                                         vk::ImageLayout::eTransferDstOptimal, region);
//         setImageLayout(commandBuffer, vk::ImageLayout::eGeneral);
//     });
// }
//
// Image::Image(const std::vector<std::string>& filepaths) {
//     unsigned char* data = loadFile(filepaths[0]);
//     stbi_image_free(data);
//
//     depth = filepaths.size();
//     type = vk::ImageType::e3D;
//
//     std::vector<uint8_t> allData(width * height * depth * 4);
//     size_t offset = 0;
//     for (const auto& filepath : filepaths) {
//         spdlog::info("Load image: {}", filepath);
//         int w, h, channels;
//         data = stbi_load(filepath.c_str(), &w, &h, &channels, sizeof(unsigned char) * 4);
//         auto size = width * height * 4;
//         std::copy_n(data, size, allData.begin() + offset);
//         offset += size;
//         stbi_image_free(data);
//     }
//
//     format = vk::Format::eR8G8B8A8Unorm;
//     usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled |
//             vk::ImageUsageFlagBits::eTransferDst;
//     aspect = vk::ImageAspectFlagBits::eColor;
//     createImage();
//     allocateMemory();
//     bindImageMemory();
//     createImageView();
//     createSampler();
//
//     Buffer staging{BufferUsage::Staging, static_cast<size_t>(width * height * depth * 4)};
//     staging.copy(allData.data());
//     Context::oneTimeSubmit([&](vk::CommandBuffer commandBuffer) {
//         vk::BufferImageCopy region;
//         region.setImageExtent({width, height, depth});
//         region.setImageSubresource({aspect, 0, 0, 1});
//         region.imageSubresource.aspectMask = aspect;
//
//         setImageLayout(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
//         commandBuffer.copyBufferToImage(staging.getBuffer(), *image,
//                                         vk::ImageLayout::eTransferDstOptimal, region);
//         setImageLayout(commandBuffer, vk::ImageLayout::eGeneral);
//     });
// }
//
// void Image::setImageLayout(vk::CommandBuffer commandBuffer, vk::ImageLayout newLayout) {
//     setImageLayout(commandBuffer, *image, newLayout, aspect);
//     layout = newLayout;
// }
//
// void Image::copyToImage(vk::CommandBuffer commandBuffer, const Image& dst) const {
//     vk::ImageCopy copyRegion;
//     copyRegion.setSrcSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});
//     copyRegion.setDstSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1});
//     copyRegion.setExtent({width, height, 1});
//
//     vk::Image srcImage = *image;
//     vk::Image dstImage = dst.getImage();
//
//     commandBuffer.copyImage(srcImage, vk::ImageLayout::eTransferSrcOptimal, dstImage,
//                             vk::ImageLayout::eTransferDstOptimal, copyRegion);
// }
//
// void Image::copyToBuffer(vk::CommandBuffer commandBuffer, Buffer& dst) {
//     vk::ImageSubresourceLayers subresource;
//     subresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
//     subresource.setBaseArrayLayer(0);
//     subresource.setLayerCount(1);
//     subresource.setMipLevel(0);
//
//     vk::BufferImageCopy copyInfo;
//     copyInfo.setBufferOffset(0);
//     copyInfo.setBufferRowLength(width);
//     copyInfo.setBufferImageHeight(height);
//     copyInfo.setImageExtent({width, height, 1});
//     copyInfo.setImageSubresource(subresource);
//
//     auto oldLayout = layout;
//     setImageLayout(commandBuffer, vk::ImageLayout::eTransferSrcOptimal);
//     commandBuffer.copyImageToBuffer(*image, vk::ImageLayout::eTransferSrcOptimal,
//     dst.getBuffer(),
//                                     copyInfo);
//     setImageLayout(commandBuffer, oldLayout);
// }
//
// void Image::save(const std::string& filepath) {
//     static vk::DeviceSize size = width * height * 4;
//     static Buffer buffer{BufferUsage::Staging, size};
//     static uint8_t* pixels = static_cast<uint8_t*>(buffer.map());
//
//     Context::oneTimeSubmit(
//         [&](vk::CommandBuffer commandBuffer) { copyToBuffer(commandBuffer, buffer); });
//
//     std::vector<uint8_t> output(width * height * 3);
//     for (int h = 0; h < height; h++) {
//         for (int w = 0; w < width; w++) {
//             int index = h * width + w;
//             output[index * 3 + 0] = pixels[index * 4 + 2];
//             output[index * 3 + 1] = pixels[index * 4 + 1];
//             output[index * 3 + 2] = pixels[index * 4 + 0];
//         }
//     }
//
//     stbi_write_png(filepath.c_str(), width, height, 3, output.data(), width * 3);
// }
//
// vk::AttachmentDescription Image::createAttachmentDesc() const {
//     vk::AttachmentDescription attachDesc;
//     attachDesc.setFormat(format);
//     attachDesc.setSamples(vk::SampleCountFlagBits::e1);
//
//     if (usage & vk::ImageUsageFlagBits::eColorAttachment) {
//         // Store on end (color image)
//         // so, explicit clear command is required. (off course you can use any clear color)
//         attachDesc.setLoadOp(vk::AttachmentLoadOp::eDontCare);
//         attachDesc.setStoreOp(vk::AttachmentStoreOp::eStore);
//         attachDesc.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);
//     } else if (usage & vk::ImageUsageFlagBits::eDepthStencilAttachment) {
//         // Clear on begin (depth image)
//         // so, there is no need to clear explicitly.
//         attachDesc.setLoadOp(vk::AttachmentLoadOp::eClear);
//         attachDesc.setStoreOp(vk::AttachmentStoreOp::eDontCare);
//         attachDesc.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
//     }
//     attachDesc.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
//     attachDesc.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
//     return attachDesc;
// }
//
// vk::AttachmentReference Image::createAttachmentRef(uint32_t attachment) const {
//     vk::AttachmentReference attachRef;
//     attachRef.setAttachment(attachment);
//     attachRef.setLayout(layout);
//     return attachRef;
// }
//
void Image::setImageLayout(vk::CommandBuffer commandBuffer,
                           vk::Image image,
                           vk::ImageLayout oldLayout,
                           vk::ImageLayout newLayout,
                           vk::ImageAspectFlags aspect,
                           uint32_t mipLevels) {
    vk::PipelineStageFlags srcStageMask = vk::PipelineStageFlagBits::eAllCommands;
    vk::PipelineStageFlags dstStageMask = vk::PipelineStageFlagBits::eAllCommands;

    vk::ImageMemoryBarrier barrier{};
    barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setImage(image);
    barrier.setOldLayout(oldLayout);
    barrier.setNewLayout(newLayout);
    barrier.setSubresourceRange({aspect, 0, mipLevels, 0, 1});

    switch (oldLayout) {
        case vk::ImageLayout::eTransferDstOptimal:
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            break;
        case vk::ImageLayout::eTransferSrcOptimal:
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
            break;
        case vk::ImageLayout::eColorAttachmentOptimal:
            barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead;
            break;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            barrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead;
            break;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
            break;
        case vk::ImageLayout::ePresentSrcKHR:
            barrier.srcAccessMask = vk::AccessFlagBits::eMemoryRead;
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
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            break;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            break;
        case vk::ImageLayout::ePresentSrcKHR:
            barrier.dstAccessMask = vk::AccessFlagBits::eMemoryRead;
            break;
        default:
            break;
    }

    commandBuffer.pipelineBarrier(srcStageMask, dstStageMask, {}, {}, {}, barrier);
}

//// private member functions
// void Image::createImage() {
//     vk::ImageCreateInfo imageInfo;
//     imageInfo.setImageType(type);
//     imageInfo.setFormat(format);
//     imageInfo.setExtent({width, height, depth});
//     imageInfo.setMipLevels(1);
//     imageInfo.setArrayLayers(1);
//     imageInfo.setSamples(vk::SampleCountFlagBits::e1);
//     imageInfo.setUsage(usage);
//     image = Context::getDevice().createImageUnique(imageInfo);
// }
//
// void Image::allocateMemory() {
//     vk::MemoryRequirements requirements =
//     Context::getDevice().getImageMemoryRequirements(*image); uint32_t memoryTypeIndex =
//         Context::findMemoryTypeIndex(requirements, vk::MemoryPropertyFlagBits::eDeviceLocal);
//     vk::MemoryAllocateInfo memoryInfo;
//     memoryInfo.setAllocationSize(requirements.size);
//     memoryInfo.setMemoryTypeIndex(memoryTypeIndex);
//     memory = Context::getDevice().allocateMemoryUnique(memoryInfo);
// }
//
// void Image::createImageView() {
//     vk::ImageViewCreateInfo viewInfo;
//     viewInfo.setImage(*image);
//     viewInfo.setFormat(format);
//     viewInfo.setSubresourceRange({aspect, 0, 1, 0, 1});
//     if (type == vk::ImageType::e2D) {
//         viewInfo.setViewType(vk::ImageViewType::e2D);
//     } else if (type == vk::ImageType::e3D) {
//         viewInfo.setViewType(vk::ImageViewType::e3D);
//     } else {
//         assert(false);
//     }
//     view = Context::getDevice().createImageViewUnique(viewInfo);
// }
//
// void Image::bindImageMemory() {
//     Context::getDevice().bindImageMemory(*image, *memory, 0);
// }
//
// void Image::createSampler() {
//     sampler = Context::getDevice().createSamplerUnique(
//         vk::SamplerCreateInfo()
//             .setMagFilter(vk::Filter::eLinear)
//             .setMinFilter(vk::Filter::eLinear)
//             .setAnisotropyEnable(VK_FALSE)
//             .setMaxLod(0.0f)
//             .setMinLod(0.0f)
//             .setMipmapMode(vk::SamplerMipmapMode::eLinear)
//             .setAddressModeU(vk::SamplerAddressMode::eRepeat)
//             .setAddressModeV(vk::SamplerAddressMode::eRepeat)
//             .setAddressModeW(vk::SamplerAddressMode::eRepeat));
// }
//
// unsigned char* Image::loadFile(const std::string& filepath) {
//     int w, h, channels;
//     unsigned char* data = stbi_load(filepath.c_str(), &w, &h, &channels, sizeof(unsigned char) *
//     4); if (!data) {
//         throw std::runtime_error("Failed to load texture: " + filepath);
//     }
//     width = w;
//     height = h;
//     return data;
// }
}  // namespace rv
