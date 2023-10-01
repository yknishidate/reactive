#pragma once
#include <spirv_glsl.hpp>
#include <unordered_map>
#include <variant>
#include "Accel.hpp"
#include "ArrayProxy.hpp"
#include "Buffer.hpp"
#include "Image.hpp"
#include "Shader.hpp"

namespace rv {
class WriteDescriptorSet {
public:
    // Buffer
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding,
                       ArrayProxy<vk::DescriptorBufferInfo> infos);

    // Image
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding,
                       ArrayProxy<vk::DescriptorImageInfo> infos);

    // TopAccel
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding,
                       ArrayProxy<vk::WriteDescriptorSetAccelerationStructureKHR> infos);

    auto get() const -> vk::WriteDescriptorSet {
        return write;
    }

private:
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding);

    vk::WriteDescriptorSet write{};
    std::vector<vk::DescriptorImageInfo> imageInfos;
    std::vector<vk::DescriptorBufferInfo> bufferInfos;
    std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> accelInfos;
};

struct DescriptorSetCreateInfo {
    ArrayProxy<ShaderHandle> shaders;
    ArrayProxy<std::pair<const char*, ArrayProxy<BufferHandle>>> buffers;
    ArrayProxy<std::pair<const char*, ArrayProxy<ImageHandle>>> images;
    ArrayProxy<std::pair<const char*, ArrayProxy<TopAccelHandle>>> accels;
};

class DescriptorSet {
    friend class CommandBuffer;

public:
    DescriptorSet(const Context* context, DescriptorSetCreateInfo createInfo);

    void allocate();
    void update();
    void bind(vk::CommandBuffer commandBuffer,
              vk::PipelineBindPoint bindPoint,
              vk::PipelineLayout pipelineLayout);

    void addResources(ShaderHandle shader);

    vk::DescriptorSetLayout getLayout() const {
        return *descSetLayout;
    }

private:
    void updateBindingMap(const spirv_cross::Resource& resource,
                          const spirv_cross::CompilerGLSL& glsl,
                          vk::ShaderStageFlags stage,
                          vk::DescriptorType type);

    const Context* context;
    vk::UniqueDescriptorSet descSet;
    vk::UniqueDescriptorSetLayout descSetLayout;
    std::unordered_map<std::string, vk::DescriptorSetLayoutBinding> bindingMap;
    std::vector<WriteDescriptorSet> writes;
};
}  // namespace rv
