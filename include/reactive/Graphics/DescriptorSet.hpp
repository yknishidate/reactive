#pragma once
#include <SPIRV-Reflect/spirv_reflect.h>
#include <unordered_map>
#include <variant>

#include "ArrayProxy.hpp"
#include "Image.hpp"
#include "Shader.hpp"

namespace rv {
struct DescriptorSetCreateInfo {
    ArrayProxy<ShaderHandle> shaders;
    ArrayProxy<std::pair<const char*, std::variant<ArrayProxy<BufferHandle>, uint32_t>>> buffers;
    ArrayProxy<std::pair<const char*, std::variant<ArrayProxy<ImageHandle>, uint32_t>>> images;
    ArrayProxy<std::pair<const char*, std::variant<ArrayProxy<TopAccelHandle>, uint32_t>>> accels;
};

class DescriptorSet {
public:
    DescriptorSet(const Context& context, const DescriptorSetCreateInfo& createInfo);

    void update();

    void set(const std::string& name, ArrayProxy<BufferHandle> buffers);
    void set(const std::string& name, ArrayProxy<ImageHandle> images);
    void set(const std::string& name, ArrayProxy<TopAccelHandle> accels);

    vk::DescriptorSetLayout getLayout() const { return *m_descSetLayout; }
    vk::DescriptorSet getDescriptorSet() const { return *m_descSet; }

private:
    void addResources(ShaderHandle shader);
    void updateBindingMap(const SpvReflectDescriptorBinding* binding, vk::ShaderStageFlags stage);

    const Context* m_context;
    vk::UniqueDescriptorSet m_descSet;
    vk::UniqueDescriptorSetLayout m_descSetLayout;

    using BufferInfos = std::vector<vk::DescriptorBufferInfo>;
    using ImageInfos = std::vector<vk::DescriptorImageInfo>;
    using AccelInfos = std::vector<vk::WriteDescriptorSetAccelerationStructureKHR>;
    struct Descriptor {
        vk::DescriptorSetLayoutBinding binding;
        std::variant<BufferInfos, ImageInfos, AccelInfos> infos;
    };

    std::unordered_map<std::string, Descriptor> m_descriptors;
};
}  // namespace rv
