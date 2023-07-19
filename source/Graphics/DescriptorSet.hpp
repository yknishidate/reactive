#pragma once
#include <spirv_glsl.hpp>
#include <unordered_map>
#include <variant>
#include "Accel.hpp"
#include "ArrayProxy.hpp"
#include "Buffer.hpp"
#include "Image.hpp"
#include "Shader.hpp"

class WriteDescriptorSet {
public:
    // Buffer
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, vk::DescriptorBufferInfo bufferInfo);
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding,
                       std::vector<vk::DescriptorBufferInfo> infos);

    // Image
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, vk::DescriptorImageInfo imageInfo);
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding,
                       std::vector<vk::DescriptorImageInfo> infos);

    // TopAccel
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding,
                       vk::WriteDescriptorSetAccelerationStructureKHR accelInfo);
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding,
                       std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> infos);

    vk::WriteDescriptorSet get() const { return write; }

private:
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding);

    vk::WriteDescriptorSet write{};
    std::vector<vk::DescriptorImageInfo> imageInfos;
    std::vector<vk::DescriptorBufferInfo> bufferInfos;
    std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> accelInfos;
};

struct DescriptorSetCreateInfo {
    ArrayProxy<Shader> shaders;
    ArrayProxy<std::pair<const char*, ArrayProxy<const Buffer>>> buffers;
    ArrayProxy<std::pair<const char*, ArrayProxy<const Image>>> images;
    ArrayProxy<std::pair<const char*, ArrayProxy<const TopAccel>>> accels;
};

class DescriptorSet {
public:
    DescriptorSet() = default;
    DescriptorSet(const Context* context, DescriptorSetCreateInfo createInfo);

    void allocate();
    void update();
    void bind(vk::CommandBuffer commandBuffer,
              vk::PipelineBindPoint bindPoint,
              vk::PipelineLayout pipelineLayout);

    void addResources(const Shader& shader);

    vk::DescriptorSetLayout getLayout() const { return *descSetLayout; }

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
