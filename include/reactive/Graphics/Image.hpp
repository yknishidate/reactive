#pragma once
#include <vulkan/vulkan.hpp>
#include "Context.hpp"

namespace rv {
class Buffer;

struct ImageCreateInfo {
    vk::ImageUsageFlags usage;
    vk::Extent3D extent = {1, 1, 1};
    vk::Format format;
    vk::ImageLayout layout;
    // if mipLevels is std::numeric_limits<uint32_t>::max(), then it's set to max level
    vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor;
    uint32_t mipLevels = 1;
    const char* debugName = nullptr;
};

class Image {
public:
    Image(const Context* context,
          vk::ImageUsageFlags usage,
          vk::Extent3D extent,
          vk::Format format,
          vk::ImageLayout layout,
          vk::ImageAspectFlags aspect,
          uint32_t mipLevels,
          const char* debugName);

    Image(vk::Image image, vk::ImageView view, vk::Extent3D extent, vk::ImageAspectFlags aspect)
        : image{image}, view{view}, extent{extent}, aspect{aspect} {}

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

    vk::Extent3D getExtent() const { return extent; }

    // Ensure that data is pre-filled
    // ImageLayout is implicitly shifted to ShaderReadOnlyOptimal
    void generateMipmaps();

    static ImageHandle loadFromFile(const Context& context,
                                    const std::string& filepath,
                                    uint32_t mipLevels = 1);

    // mipmap is not supported
    static ImageHandle loadFromFileHDR(const Context& context, const std::string& filepath);

    void transitionLayout(vk::CommandBuffer commandBuffer, vk::ImageLayout newLayout);

private:
    const Context* context;
    vk::Image image;
    vk::DeviceMemory memory;
    vk::ImageView view;
    vk::Sampler sampler;
    bool hasOwnership = false;

    vk::ImageLayout layout = vk::ImageLayout::eUndefined;
    vk::Format format;
    vk::Extent3D extent;
    uint32_t mipLevels = 1;
    vk::ImageAspectFlags aspect;
};
}  // namespace rv
