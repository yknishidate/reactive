#pragma once
#include <unordered_map>
#include <spirv_glsl.hpp>
#include "Buffer.hpp"
#include "Image.hpp"
#include "Accel.hpp"
#include "WriteDescriptorSet.hpp"

class DescriptorSet
{
public:
    void setup();
    void update();
    void bind(vk::CommandBuffer commandBuffer, vk::PipelineBindPoint bindPoint, vk::PipelineLayout pipelineLayout);

    void record(const std::string& name, const std::vector<Image>& images);
    void record(const std::string& name, const Buffer& buffer);
    void record(const std::string& name, const Image& image);
    void record(const std::string& name, const TopAccel& accel);

    void addBindingMap(const std::vector<uint32_t>& spvShader, vk::ShaderStageFlags stage);

    std::vector<vk::DescriptorSetLayoutBinding> getBindings() const;

    vk::UniquePipelineLayout createPipelineLayout(size_t pushSize, vk::ShaderStageFlags shaderStage) const;

private:
    void updateBindingMap(spirv_cross::Resource& resource, spirv_cross::CompilerGLSL& glsl,
                          vk::ShaderStageFlags stage, vk::DescriptorType type);

    vk::UniqueDescriptorSet descSet;
    vk::UniqueDescriptorSetLayout descSetLayout;
    std::unordered_map<std::string, vk::DescriptorSetLayoutBinding> bindingMap;
    std::vector<WriteDescriptorSet> writes;
};
