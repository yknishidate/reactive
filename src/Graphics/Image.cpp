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
Image::Image(const Context& _context, const ImageCreateInfo& createInfo)
    // NOTE: layout is updated by transitionLayout after this ctor.
    : context{&_context},
      debugName{createInfo.debugName},
      hasOwnership{true},
      extent{createInfo.extent},
      format{createInfo.format},
      mipLevels{createInfo.mipLevels} {
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
    imageInfo.setArrayLayers(layerCount);
    image = context->getDevice().createImage(imageInfo);

    vk::MemoryRequirements requirements = context->getDevice().getImageMemoryRequirements(image);
    uint32_t memoryTypeIndex = context->findMemoryTypeIndex(  //
        requirements, vk::MemoryPropertyFlagBits::eDeviceLocal);

    vk::MemoryAllocateInfo memoryInfo;
    memoryInfo.setAllocationSize(requirements.size);
    memoryInfo.setMemoryTypeIndex(memoryTypeIndex);
    memory = context->getDevice().allocateMemory(memoryInfo);

    context->getDevice().bindImageMemory(image, memory, 0);

    if (!debugName.empty()) {
        context->setDebugName(image, createInfo.debugName.c_str());
    }
}

// KTX から読み取った情報をもとに作成する
// ImageView と Sampler の作成はアプリ側
Image::Image(const Context* _context,
             vk::Image _image,
             vk::Format _imageFormat,
             vk::ImageLayout _imageLayout,
             vk::DeviceMemory _deviceMemory,
             vk::ImageViewType _viewType,
             uint32_t _width,
             uint32_t _height,
             uint32_t _depth,
             uint32_t _levelCount,
             uint32_t _layerCount)
    : context{_context},
      image{_image},
      memory{_deviceMemory},
      viewType{_viewType},
      hasOwnership{true},
      layout{_imageLayout},
      extent{_width, _height, _depth},
      format{_imageFormat},
      mipLevels{_levelCount},
      layerCount{_layerCount} {}

Image::~Image() {
    if (hasOwnership) {
        if (sampler) {
            context->getDevice().destroySampler(sampler);
        }
        if (view) {
            context->getDevice().destroyImageView(view);
        }
        context->getDevice().freeMemory(memory);
        context->getDevice().destroyImage(image);
    }
}

ImageHandle Image::loadFromFile(const Context& context,
                                const std::filesystem::path& filepath,
                                uint32_t mipLevels,
                                vk::Filter filter,
                                vk::SamplerAddressMode addressMode) {
    std::string filepathStr = filepath.string();
    int width;
    int height;
    int comp = 4;
    unsigned char* pixels = stbi_load(filepathStr.c_str(), &width, &height, nullptr, comp);
    if (!pixels) {
        throw std::runtime_error("Failed to load image: " + filepathStr);
    }

    ImageHandle image = context.createImage({
        .usage = ImageUsage::Sampled,
        .extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
        .format = vk::Format::eR8G8B8A8Unorm,
        .mipLevels = mipLevels,
        .debugName = filepathStr,
    });
    image->createImageView();
    image->createSampler(filter, addressMode);

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

        // TODO: refactor
        vk::ImageLayout newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        if (mipLevels > 1) {
            newLayout = vk::ImageLayout::eTransferSrcOptimal;
        }
        commandBuffer->transitionLayout(image, newLayout);
        if (mipLevels > 1) {
            image->generateMipmaps(*commandBuffer);
        }
    });

    stbi_image_free(pixels);

    return image;
}

ImageHandle Image::loadFromFileHDR(const Context& context, const std::filesystem::path& filepath) {
    std::string filepathStr = filepath.string();
    int width;
    int height;
    int comp = 4;
    float* pixels = stbi_loadf(filepathStr.c_str(), &width, &height, nullptr, comp);
    if (!pixels) {
        throw std::runtime_error("Failed to load image: " + filepathStr);
    }

    ImageHandle image = context.createImage({
        .usage = ImageUsage::Sampled,
        .extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
        .format = vk::Format::eR32G32B32A32Sfloat,
    });
    image->createImageView();
    image->createSampler();

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

auto Image::loadFromKTX(const Context& context, const std::filesystem::path& filepath)
    -> ImageHandle {
    std::string filepathStr = filepath.string();
    ktxVulkanDeviceInfo kvdi;
    ktxTexture* kTexture;
    ktxVulkanTexture texture;

    ktxVulkanDeviceInfo_Construct(&kvdi, context.getPhysicalDevice(), context.getDevice(),
                                  context.getQueue(), context.getCommandPool(), nullptr);

    KTX_error_code result =
        ktxTexture_CreateFromNamedFile(filepathStr.c_str(), KTX_TEXTURE_CREATE_NO_FLAGS, &kTexture);
    if (KTX_SUCCESS != result) {
        std::stringstream message;
        message << "Creation of ktxTexture from "  //
                << filepathStr << " failed: "      //
                << ktxErrorString(result);
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

    ImageHandle image = std::make_shared<Image>(            //
        &context,                                           //
        texture.image,                                      //
        static_cast<vk::Format>(texture.imageFormat),       //
        static_cast<vk::ImageLayout>(texture.imageLayout),  //
        texture.deviceMemory,                               //
        static_cast<vk::ImageViewType>(texture.viewType),   //
        texture.width, texture.height, texture.depth,       //
        texture.levelCount, texture.layerCount);
    image->createImageView(static_cast<vk::ImageViewType>(texture.viewType));
    image->createSampler();

    ktxTexture_Destroy(kTexture);
    ktxVulkanDeviceInfo_Destruct(&kvdi);

    return image;
}

void Image::generateMipmaps(const CommandBuffer& commandBuffer) {
    RV_ASSERT(mipLevels > 1, "mipLevels is not set greater than 1 when the image is created.");

    commandBuffer.beginDebugLabel("GenerateMipmap");
    vk::ImageLayout oldLayout = layout;
    vk::ImageLayout newLayout = layout;

    // Check if image format supports linear blitting
    vk::Filter filter = vk::Filter::eLinear;
    vk::FormatProperties formatProperties =
        context->getPhysicalDevice().getFormatProperties(format);

    bool isLinearFilteringSupported =
        static_cast<bool>(formatProperties.optimalTilingFeatures &
                          vk::FormatFeatureFlagBits::eSampledImageFilterLinear);
    bool isFormatDepthStencil =
        format == vk::Format::eD16Unorm || format == vk::Format::eD16UnormS8Uint ||
        format == vk::Format::eD24UnormS8Uint || format == vk::Format::eD32Sfloat ||
        format == vk::Format::eD32SfloatS8Uint;
    if (isFormatDepthStencil || !isLinearFilteringSupported) {
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
    vk::ImageMemoryBarrier barrier{};
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = aspect;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = extent.width;
    int32_t mipHeight = extent.height;

    for (uint32_t i = 1; i < mipLevels; i++) {
        // Src (i - 1)
        {
            // TODO: access maskをしっかり設定する
            // NOTE: level = 0 についてはまだ遷移していないので old = 既存のレイアウトとする
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = i == 1 ? oldLayout : vk::ImageLayout::eTransferDstOptimal;
            barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
            commandBuffer.imageBarrier(barrier, vk::PipelineStageFlagBits::eTransfer,
                                       vk::PipelineStageFlagBits::eTransfer);
        }
        // Dst (i)
        {
            // NOTE: Dst はこれから書きこまれるので old = undef でよい
            // ただし省略すると古い情報が残るので上書きすること
            barrier.subresourceRange.baseMipLevel = i;
            barrier.oldLayout = vk::ImageLayout::eUndefined;
            barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
            commandBuffer.imageBarrier(barrier, vk::PipelineStageFlagBits::eTransfer,
                                       vk::PipelineStageFlagBits::eTransfer);
        }

        vk::ImageBlit blit{};
        blit.srcOffsets[0] = vk::Offset3D{0, 0, 0};
        blit.srcOffsets[1] = vk::Offset3D{mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = aspect;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
        blit.dstOffsets[1] = vk::Offset3D{mipWidth > 1 ? mipWidth / 2 : 1,  //
                                          mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = aspect;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        commandBuffer.commandBuffer->blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image,
                                               vk::ImageLayout::eTransferDstOptimal, blit, filter);

        if (mipWidth > 1)
            mipWidth /= 2;
        if (mipHeight > 1)
            mipHeight /= 2;
    }

    // レベル[0, 1, 2, ..., N-1, N] のうち、[0, N-1]までをまとめて遷移
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.newLayout = newLayout;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    commandBuffer.imageBarrier(barrier, vk::PipelineStageFlagBits::eTransfer,
                               vk::PipelineStageFlagBits::eAllCommands);

    // レベルNを遷移
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.subresourceRange.levelCount = 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = newLayout;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    commandBuffer.imageBarrier(barrier, vk::PipelineStageFlagBits::eTransfer,
                               vk::PipelineStageFlagBits::eAllCommands);

    commandBuffer.endDebugLabel();

    layout = newLayout;
}
}  // namespace rv
