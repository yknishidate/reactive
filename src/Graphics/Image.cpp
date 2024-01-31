#include "Graphics/Image.hpp"
#include "Graphics/Buffer.hpp"
#include "Graphics/CommandBuffer.hpp"
#include "common.hpp"

namespace {
uint32_t calculateMipLevels(uint32_t width, uint32_t height) {
    return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}
}  // namespace

namespace rv {
Image::Image(const Context* context,
             vk::ImageUsageFlags usage,
             vk::Extent3D extent,
             vk::Format format,
             vk::ImageAspectFlags aspect,
             uint32_t mipLevels,
             const char* debugName)
    // NOTE: layout is updated by transitionLayout after this ctor.
    : context{context},
      hasOwnership{true},
      format{format},
      extent{extent},
      mipLevels{mipLevels},
      aspect{aspect} {
    vk::ImageType type = extent.depth == 1 ? vk::ImageType::e2D : vk::ImageType::e3D;

    // Compute mipmap level
    if (mipLevels == std::numeric_limits<uint32_t>::max()) {
        mipLevels = calculateMipLevels(extent.width, extent.height);
    }

    // NOTE: initialLayout must be Undefined or PreInitialized
    // NOTE: queueFamily is ignored if sharingMode is not concurrent
    vk::ImageCreateInfo imageInfo;
    imageInfo.setImageType(type);
    imageInfo.setFormat(format);
    imageInfo.setExtent(extent);
    imageInfo.setMipLevels(mipLevels);
    imageInfo.setArrayLayers(1);
    imageInfo.setSamples(vk::SampleCountFlagBits::e1);
    imageInfo.setUsage(usage);
    image = context->getDevice().createImage(imageInfo);

    vk::MemoryRequirements requirements = context->getDevice().getImageMemoryRequirements(image);
    uint32_t memoryTypeIndex =
        context->findMemoryTypeIndex(requirements, vk::MemoryPropertyFlagBits::eDeviceLocal);
    vk::MemoryAllocateInfo memoryInfo;
    memoryInfo.setAllocationSize(requirements.size);
    memoryInfo.setMemoryTypeIndex(memoryTypeIndex);
    memory = context->getDevice().allocateMemory(memoryInfo);

    context->getDevice().bindImageMemory(image, memory, 0);

    vk::ImageSubresourceRange subresourceRange;
    subresourceRange.setAspectMask(aspect);
    subresourceRange.setBaseMipLevel(0);
    subresourceRange.setLevelCount(mipLevels);
    subresourceRange.setBaseArrayLayer(0);
    subresourceRange.setLayerCount(1);

    vk::ImageViewCreateInfo viewInfo;
    viewInfo.setImage(image);
    viewInfo.setFormat(format);
    viewInfo.setSubresourceRange(subresourceRange);
    if (type == vk::ImageType::e2D) {
        viewInfo.setViewType(vk::ImageViewType::e2D);
    } else if (type == vk::ImageType::e3D) {
        viewInfo.setViewType(vk::ImageViewType::e3D);
    } else {
        assert(false);
    }

    view = context->getDevice().createImageView(viewInfo);

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
    samplerInfo.setCompareEnable(VK_TRUE);
    samplerInfo.setCompareOp(vk::CompareOp::eLess);
    sampler = context->getDevice().createSampler(samplerInfo);

    if (debugName) {
        context->setDebugName(image, debugName);
        context->setDebugName(view, (std::string(debugName) + "(ImageView)").c_str());
        context->setDebugName(sampler, (std::string(debugName) + "(Sampler)").c_str());
    }
}

Image::~Image() {
    if (hasOwnership) {
        context->getDevice().destroySampler(sampler);
        context->getDevice().destroyImageView(view);
        context->getDevice().freeMemory(memory);
        context->getDevice().destroyImage(image);
    }
}

ImageHandle Image::loadFromFile(const Context& context,
                                const std::string& filepath,
                                uint32_t mipLevels) {
    int width;
    int height;
    int comp = 4;
    unsigned char* pixels = stbi_load(filepath.c_str(), &width, &height, nullptr, comp);
    if (!pixels) {
        throw std::runtime_error("Failed to load image: " + filepath);
    }

    ImageHandle image = context.createImage({
        .usage = ImageUsage::Sampled,
        .extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
        .format = vk::Format::eR8G8B8A8Unorm,
        .mipLevels = mipLevels,
        .debugName = filepath.c_str(),
    });

    // Copy to image
    BufferHandle stagingBuffer = context.createBuffer({
        .usage = BufferUsage::Staging,
        .memory = MemoryUsage::Host,
        .size = width * height * comp * sizeof(unsigned char),
    });
    stagingBuffer->copy(pixels);

    context.oneTimeSubmit([&](CommandBufferHandle commandBuffer) {
        commandBuffer->transitionLayout(image, vk::ImageLayout::eTransferDstOptimal);
        commandBuffer->copyBufferToImage(stagingBuffer, image);

        vk::ImageLayout newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        if (mipLevels > 1) {
            newLayout = vk::ImageLayout::eTransferSrcOptimal;
        }
        commandBuffer->transitionLayout(image, newLayout);
    });

    if (mipLevels > 1) {
        image->generateMipmaps();
    }

    stbi_image_free(pixels);

    return image;
}

ImageHandle Image::loadFromFileHDR(const Context& context, const std::string& filepath) {
    int width;
    int height;
    int comp = 4;
    float* pixels = stbi_loadf(filepath.c_str(), &width, &height, nullptr, comp);
    if (!pixels) {
        throw std::runtime_error("Failed to load image: " + filepath);
    }

    ImageHandle image = context.createImage({
        .usage = ImageUsage::Sampled,
        .extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
        .format = vk::Format::eR32G32B32A32Sfloat,
    });

    // Copy to image
    BufferHandle stagingBuffer = context.createBuffer({
        .usage = BufferUsage::Staging,
        .memory = MemoryUsage::Host,
        .size = width * height * comp * sizeof(float),
    });
    stagingBuffer->copy(pixels);

    context.oneTimeSubmit([&](CommandBufferHandle commandBuffer) {
        commandBuffer->transitionLayout(image, vk::ImageLayout::eTransferDstOptimal);
        commandBuffer->copyBufferToImage(stagingBuffer, image);
        commandBuffer->transitionLayout(image, vk::ImageLayout::eShaderReadOnlyOptimal);
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
    // TODO: move to command buffer
    context->oneTimeSubmit([&](CommandBufferHandle commandBuffer) {
        vk::ImageMemoryBarrier barrier{};
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = extent.width;
        int32_t mipHeight = extent.height;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

            commandBuffer->imageBarrier(barrier, vk::PipelineStageFlagBits::eTransfer,
                                        vk::PipelineStageFlagBits::eTransfer);

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

            commandBuffer->commandBuffer->blitImage(image, vk::ImageLayout::eTransferSrcOptimal,
                                                    image, vk::ImageLayout::eTransferDstOptimal,
                                                    blit, filter);

            barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            commandBuffer->imageBarrier(barrier, vk::PipelineStageFlagBits::eTransfer,
                                        vk::PipelineStageFlagBits::eFragmentShader);

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

        commandBuffer->imageBarrier(barrier, vk::PipelineStageFlagBits::eTransfer,
                                    vk::PipelineStageFlagBits::eAllCommands);
    });
}
}  // namespace rv
