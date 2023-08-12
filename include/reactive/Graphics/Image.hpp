#pragma once
#include <vulkan/vulkan.hpp>
#include "Context.hpp"

namespace rv {
class Buffer;

class Image {
public:
    Image(const Context* context,
          vk::ImageUsageFlags usage,
          uint32_t width,
          uint32_t height,
          uint32_t depth,
          vk::Format format,
          vk::ImageLayout layout,
          vk::ImageAspectFlags aspect,
          uint32_t mipLevels);

    Image(vk::Image image, vk::ImageView view) : image{image}, view{view} {}

    ~Image() {
        if (hasOwnership) {
            context->getDevice().destroySampler(sampler);
            context->getDevice().destroyImageView(view);
            context->getDevice().freeMemory(memory);
            context->getDevice().destroyImage(image);
        }
    }

    vk::Image getImage() const { return image; }

    vk::ImageView getView() const { return view; }

    vk::Sampler getSampler() const { return sampler; }

    vk::DescriptorImageInfo getInfo() const { return {sampler, view, layout}; }

    uint32_t getMipLevels() const { return mipLevels; }

    vk::ImageAspectFlags getAspectMask() const { return aspect; }

    vk::ImageLayout getLayout() const { return layout; }

    // Ensure that data is pre-filled
    // ImageLayout is implicitly shifted to ShaderReadOnlyOptimal
    void generateMipmaps();

    static ImageHandle loadFromFile(const Context& context,
                                    const std::string& filepath,
                                    uint32_t mipLevels);

    // mipmap is not supported
    static ImageHandle loadFromFileHDR(const Context& context, const std::string& filepath);

    static void transitionLayout(vk::CommandBuffer commandBuffer,
                                 vk::Image image,
                                 vk::ImageLayout oldLayout,
                                 vk::ImageLayout newLayout,
                                 vk::ImageAspectFlags aspect,
                                 uint32_t mipLevels);

private:
    const Context* context;
    vk::Image image;
    vk::DeviceMemory memory;
    vk::ImageView view;
    vk::Sampler sampler;
    bool hasOwnership = false;

    vk::ImageLayout layout = vk::ImageLayout::eUndefined;
    vk::Format format;
    uint32_t width;
    uint32_t height;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;
    vk::ImageAspectFlags aspect;
};
}  // namespace rv
