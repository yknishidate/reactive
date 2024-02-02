#pragma once
#include <vulkan/vulkan.hpp>
#include "Context.hpp"

namespace rv {
class Buffer;

struct ImageCreateInfo {
    vk::ImageUsageFlags usage;

    vk::Extent3D extent = {1, 1, 1};

    vk::Format format;

    vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor;

    // if mipLevels is std::numeric_limits<uint32_t>::max(), then it's set to max level
    uint32_t mipLevels = 1;

    bool isCubemap = false;

    const char* debugName = nullptr;
};

class Image {
    friend class CommandBuffer;

public:
    Image(const Context* context,
          vk::ImageUsageFlags usage,
          vk::Extent3D extent,
          vk::Format format,
          vk::ImageAspectFlags aspect,
          uint32_t mipLevels,
          bool isCubemap,
          const char* debugName);

    Image(vk::Image image,
          vk::ImageView view,
          vk::Extent3D extent,
          vk::Format format,
          vk::ImageAspectFlags aspect)
        : image{image}, view{view}, extent{extent}, format{format}, aspect{aspect} {}

    ~Image();

    auto getImage() const -> vk::Image { return image; }
    auto getView() const -> vk::ImageView { return view; }
    auto getSampler() const -> vk::Sampler { return sampler; }
    auto getInfo() const -> vk::DescriptorImageInfo { return {sampler, view, layout}; }
    auto getMipLevels() const -> uint32_t { return mipLevels; }
    auto getAspectMask() const -> vk::ImageAspectFlags { return aspect; }
    auto getLayout() const -> vk::ImageLayout { return layout; }
    auto getExtent() const -> vk::Extent3D { return extent; }
    auto getFormat() const -> vk::Format { return format; }

    // Ensure that data is pre-filled
    // ImageLayout is implicitly shifted to ShaderReadOnlyOptimal
    void generateMipmaps();

    // TODO: refactor these
    static auto loadFromFile(const Context& context,
                             const std::string& filepath,
                             uint32_t mipLevels = 1) -> ImageHandle;

    // mipmap is not supported
    static auto loadFromFileHDR(const Context& context, const std::string& filepath) -> ImageHandle;

    static auto loadFromKTX(const Context& context, const std::string& filepath, vk::Format format)
        -> ImageHandle;

private:
    const Context* context = nullptr;

    vk::Image image;
    vk::DeviceMemory memory;
    vk::ImageView view;
    vk::Sampler sampler;

    bool hasOwnership = false;

    vk::ImageLayout layout = vk::ImageLayout::eUndefined;
    vk::Extent3D extent;
    vk::Format format = {};

    uint32_t mipLevels = 1;

    vk::ImageAspectFlags aspect;
};
}  // namespace rv
