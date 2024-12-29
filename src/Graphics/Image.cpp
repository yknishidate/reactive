#include "reactive/Graphics/Image.hpp"

#include <ktx.h>
#include <ktxvulkan.h>

#include "reactive/Graphics/Buffer.hpp"
#include "reactive/Graphics/CommandBuffer.hpp"
#include "reactive/common.hpp"

namespace {
uint32_t calculateMipLevels(uint32_t width, uint32_t height) {
    return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}
}  // namespace

namespace rv {
Image::Image(const Context& context, const ImageCreateInfo& createInfo)
    // NOTE: layout is updated by transitionLayout after this ctor.
    : m_context{&context},
      m_debugName{createInfo.debugName},
      m_hasOwnership{true},
      m_extent{createInfo.extent},
      m_format{createInfo.format},
      m_mipLevels{createInfo.mipLevels} {
    // Compute mipmap level
    if (m_mipLevels == std::numeric_limits<uint32_t>::max()) {
        m_mipLevels = calculateMipLevels(m_extent.width, m_extent.height);
    }

    // NOTE: initialLayout must be Undefined or PreInitialized
    // NOTE: queueFamily is ignored if sharingMode is not concurrent
    vk::ImageCreateInfo imageInfo;
    imageInfo.setImageType(createInfo.imageType);
    imageInfo.setFormat(m_format);
    imageInfo.setExtent(m_extent);
    imageInfo.setMipLevels(m_mipLevels);
    imageInfo.setSamples(vk::SampleCountFlagBits::e1);
    imageInfo.setUsage(createInfo.usage);
    imageInfo.setArrayLayers(m_layerCount);
    m_image = m_context->getDevice().createImage(imageInfo);

    vk::MemoryRequirements requirements = m_context->getDevice().getImageMemoryRequirements(m_image);
    uint32_t memoryTypeIndex = m_context->findMemoryTypeIndex(  //
        requirements, vk::MemoryPropertyFlagBits::eDeviceLocal);

    vk::MemoryAllocateInfo memoryInfo;
    memoryInfo.setAllocationSize(requirements.size);
    memoryInfo.setMemoryTypeIndex(memoryTypeIndex);
    m_memory = m_context->getDevice().allocateMemory(memoryInfo);

    m_context->getDevice().bindImageMemory(m_image, m_memory, 0);

    // Image view
    if (createInfo.viewInfo.has_value()) {
        switch (createInfo.imageType) {
            case vk::ImageType::e1D: {
                m_viewType = vk::ImageViewType::e1D;
                break;
            }
            case vk::ImageType::e2D: {
                m_viewType = vk::ImageViewType::e2D;
                break;
            }
            case vk::ImageType::e3D: {
                m_viewType = vk::ImageViewType::e3D;
                break;
            }
            default: {
                m_viewType = {};
                break;
            }
        }
        createImageView(m_viewType, createInfo.viewInfo.value().aspect);
    }

    // Sampler
    if (createInfo.samplerInfo.has_value()) {
        createSampler(createInfo.samplerInfo.value().filter,
                      createInfo.samplerInfo.value().addressMode,
                      createInfo.samplerInfo.value().mipmapMode);
    }

    // Debug
    if (!m_debugName.empty()) {
        m_context->setDebugName(m_image, createInfo.debugName.c_str());
    }
}

// KTX から読み取った情報をもとに作成する
// ImageView と Sampler の作成はアプリ側
Image::Image(const Context* context,
             vk::Image image,
             vk::Format imageFormat,
             vk::ImageLayout imageLayout,
             vk::DeviceMemory deviceMemory,
             vk::ImageViewType viewType,
             uint32_t width,
             uint32_t height,
             uint32_t depth,
             uint32_t levelCount,
             uint32_t layerCount)
    : m_context{context},
      m_image{image},
      m_memory{deviceMemory},
      m_viewType{viewType},
      m_hasOwnership{true},
      m_layout{imageLayout},
      m_extent{width, height, depth},
      m_format{imageFormat},
      m_mipLevels{levelCount},
      m_layerCount{layerCount} {}

Image::~Image() {
    if (m_hasOwnership) {
        if (m_sampler) {
            m_context->getDevice().destroySampler(m_sampler);
        }
        if (m_view) {
            m_context->getDevice().destroyImageView(m_view);
        }
        m_context->getDevice().freeMemory(m_memory);
        m_context->getDevice().destroyImage(m_image);
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
        throw std::runtime_error("Failed to load m_image: " + filepathStr);
    }

    ImageHandle image = context.createImage({
        .usage = ImageUsage::Sampled,
        .extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
        .format = vk::Format::eR8G8B8A8Unorm,
        .mipLevels = mipLevels,
        .viewInfo = ImageViewCreateInfo{},
        .samplerInfo =
            SamplerCreateInfo{
                .filter = filter,
                .addressMode = addressMode,
            },
        .debugName = filepathStr,
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
        throw std::runtime_error("Failed to load m_image: " + filepathStr);
    }

    ImageHandle image = context.createImage({
        .usage = ImageUsage::Sampled,
        .extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
        .format = vk::Format::eR32G32B32A32Sfloat,
        .viewInfo = ImageViewCreateInfo{},
        .samplerInfo = SamplerCreateInfo{},
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
    image->createImageView(static_cast<vk::ImageViewType>(texture.viewType),
                           vk::ImageAspectFlagBits::eColor);
    image->createSampler(vk::Filter::eLinear, vk::SamplerAddressMode::eRepeat,
                         vk::SamplerMipmapMode ::eNearest);

    ktxTexture_Destroy(kTexture);
    ktxVulkanDeviceInfo_Destruct(&kvdi);

    return image;
}

void Image::generateMipmaps(const CommandBuffer& commandBuffer) {
    RV_ASSERT(m_mipLevels > 1, "m_mipLevels is not set greater than 1 when the m_image is created.");

    commandBuffer.beginDebugLabel("GenerateMipmap");
    vk::ImageLayout oldLayout = m_layout;
    vk::ImageLayout newLayout = m_layout;

    // Check if image format supports linear blitting
    vk::Filter filter = vk::Filter::eLinear;
    vk::FormatProperties formatProperties =
        m_context->getPhysicalDevice().getFormatProperties(m_format);

    bool isLinearFilteringSupported =
        static_cast<bool>(formatProperties.optimalTilingFeatures &
                          vk::FormatFeatureFlagBits::eSampledImageFilterLinear);
    bool isFormatDepthStencil =
        m_format == vk::Format::eD16Unorm || m_format == vk::Format::eD16UnormS8Uint ||
        m_format == vk::Format::eD24UnormS8Uint || m_format == vk::Format::eD32Sfloat ||
        m_format == vk::Format::eD32SfloatS8Uint;
    if (isFormatDepthStencil || !isLinearFilteringSupported) {
        filter = vk::Filter::eNearest;
    }
    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitSrc)) {
        RV_ASSERT(false, "This m_format does not suppoprt blitting: {}", vk::to_string(m_format));
    }
    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst)) {
        RV_ASSERT(false, "This m_format does not suppoprt blitting: {}", vk::to_string(m_format));
    }

    // TODO: support 3D
    // TODO: move to command buffer
    vk::ImageMemoryBarrier barrier{};
    barrier.image = m_image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = m_aspect;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = m_extent.width;
    int32_t mipHeight = m_extent.height;

    for (uint32_t i = 1; i < m_mipLevels; i++) {
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
        blit.srcSubresource.aspectMask = m_aspect;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
        blit.dstOffsets[1] = vk::Offset3D{mipWidth > 1 ? mipWidth / 2 : 1,  //
                                          mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = m_aspect;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        commandBuffer.m_commandBuffer->blitImage(m_image, vk::ImageLayout::eTransferSrcOptimal, m_image,
                                               vk::ImageLayout::eTransferDstOptimal, blit, filter);

        if (mipWidth > 1)
            mipWidth /= 2;
        if (mipHeight > 1)
            mipHeight /= 2;
    }

    // レベル[0, 1, 2, ..., N-1, N] のうち、[0, N-1]までをまとめて遷移
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = m_mipLevels - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.newLayout = newLayout;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    commandBuffer.imageBarrier(barrier, vk::PipelineStageFlagBits::eTransfer,
                               vk::PipelineStageFlagBits::eAllCommands);

    // レベルNを遷移
    barrier.subresourceRange.baseMipLevel = m_mipLevels - 1;
    barrier.subresourceRange.levelCount = 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = newLayout;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    commandBuffer.imageBarrier(barrier, vk::PipelineStageFlagBits::eTransfer,
                               vk::PipelineStageFlagBits::eAllCommands);

    commandBuffer.endDebugLabel();

    m_layout = newLayout;
}
}  // namespace rv
