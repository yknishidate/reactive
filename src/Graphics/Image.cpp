#include "Graphics/Image.hpp"

#include <ktx.h>
#include <ktxvulkan.h>

#include "Graphics/Buffer.hpp"
#include "Graphics/CommandBuffer.hpp"
#include "common.hpp"

namespace {
uint32_t calculateMipLevels(uint32_t width, uint32_t height) {
    return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}
}  // namespace

namespace rv {
Image::Image(const Context* context, ImageCreateInfo createInfo)
    // NOTE: layout is updated by transitionLayout after this ctor.
    : context{context},
      hasOwnership{true},
      extent{createInfo.extent},
      format{createInfo.format},
      mipLevels{createInfo.mipLevels},
      aspect{createInfo.aspect} {
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
    imageInfo.setSamples(vk::SampleCountFlagBits::e1);
    imageInfo.setUsage(createInfo.usage);
    if (createInfo.isCubemap) {
        layerCount = 6;
        imageInfo.setFlags(vk::ImageCreateFlagBits::eCubeCompatible);
    }
    imageInfo.setArrayLayers(layerCount);
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
    if (createInfo.isCubemap) {
        subresourceRange.setLayerCount(6);
    }

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
    if (createInfo.isCubemap) {
        viewInfo.setViewType(vk::ImageViewType::eCube);
    }

    view = context->getDevice().createImageView(viewInfo);

    vk::SamplerCreateInfo samplerInfo;
    samplerInfo.setMagFilter(vk::Filter::eLinear);
    samplerInfo.setMinFilter(vk::Filter::eLinear);
    samplerInfo.setAnisotropyEnable(VK_FALSE);
    samplerInfo.setMaxLod(0.0f);
    samplerInfo.setMinLod(0.0f);
    if (mipLevels > 1) {
        samplerInfo.setMaxLod(static_cast<float>(mipLevels));
    }
    samplerInfo.setMipmapMode(vk::SamplerMipmapMode::eLinear);
    samplerInfo.setAddressModeU(vk::SamplerAddressMode::eRepeat);
    samplerInfo.setAddressModeV(vk::SamplerAddressMode::eRepeat);
    samplerInfo.setAddressModeW(vk::SamplerAddressMode::eRepeat);
    samplerInfo.setCompareEnable(VK_TRUE);
    samplerInfo.setCompareOp(vk::CompareOp::eLess);
    sampler = context->getDevice().createSampler(samplerInfo);

    if (createInfo.debugName) {
        context->setDebugName(image, createInfo.debugName);
        context->setDebugName(view, (std::string(createInfo.debugName) + "(ImageView)").c_str());
        context->setDebugName(sampler, (std::string(createInfo.debugName) + "(Sampler)").c_str());
    }
}

Image::Image(const Context* _context,
             vk::Image _image,
             vk::Format _imageFormat,
             vk::ImageLayout _imageLayout,
             vk::DeviceMemory _deviceMemory,
             uint32_t _width,
             uint32_t _height,
             uint32_t _depth,
             uint32_t _levelCount,
             uint32_t _layerCount)
    : context{_context},
      image{_image},
      memory{_deviceMemory},
      hasOwnership{true},
      layout{_imageLayout},
      extent{_width, _height, _depth},
      format{_imageFormat},
      mipLevels{_levelCount},
      layerCount{_layerCount},
      aspect{vk::ImageAspectFlagBits::eColor}  // TODO: fix
{}

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

auto Image::loadFromKTX(const Context& context, const std::string& filepath) -> ImageHandle {
    ktxVulkanDeviceInfo kvdi;
    ktxTexture* kTexture;
    ktxVulkanTexture texture;

    ktxVulkanDeviceInfo_Construct(&kvdi, context.getPhysicalDevice(), context.getDevice(),
                                  context.getQueue(), context.getCommandPool(), nullptr);

    KTX_error_code result =
        ktxTexture_CreateFromNamedFile(filepath.c_str(), KTX_TEXTURE_CREATE_NO_FLAGS, &kTexture);
    if (KTX_SUCCESS != result) {
        std::stringstream message;

        message << "Creation of ktxTexture from \"" << filepath
                << "\" failed: " << ktxErrorString(result);
        throw std::runtime_error(message.str());
    }
    result =
        ktxTexture_VkUploadEx(kTexture, &kvdi, &texture, VK_IMAGE_TILING_OPTIMAL,
                              VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    if (KTX_SUCCESS != result) {
        std::stringstream message;

        message << "ktxTexture_VkUpload failed: " << ktxErrorString(result);
        throw std::runtime_error(message.str());
    }

    ImageHandle image = std::make_shared<Image>(
        &context, texture.image, static_cast<vk::Format>(texture.imageFormat),
        static_cast<vk::ImageLayout>(texture.imageLayout), texture.deviceMemory, texture.width,
        texture.height, texture.depth, texture.levelCount, texture.layerCount);

    ktxTexture_Destroy(kTexture);
    ktxVulkanDeviceInfo_Destruct(&kvdi);

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
