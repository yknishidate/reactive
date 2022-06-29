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
    void SetupLayout();
    void Allocate();
    void Update();
    void Bind(vk::CommandBuffer commandBuffer, vk::PipelineBindPoint bindPoint, vk::PipelineLayout pipelineLayout);

    void Register(const std::string& name, const std::vector<Image>& images);
    void Register(const std::string& name, const Buffer& buffer);
    void Register(const std::string& name, const Image& image);
    void Register(const std::string& name, const TopAccel& accel);

    void AddBindingMap(const std::vector<uint32_t>& spvShader, vk::ShaderStageFlags stage);

    std::vector<vk::DescriptorSetLayoutBinding> GetBindings() const;

    vk::UniquePipelineLayout CreatePipelineLayout(size_t pushSize, vk::ShaderStageFlags shaderStage) const;

private:
    void UpdateBindingMap(spirv_cross::Resource& resource, spirv_cross::CompilerGLSL& glsl,
                          vk::ShaderStageFlags stage, vk::DescriptorType type);

    vk::UniqueDescriptorSet descSet;
    vk::UniqueDescriptorSetLayout descSetLayout;
    std::unordered_map<std::string, vk::DescriptorSetLayoutBinding> bindingMap;
    std::vector<WriteDescriptorSet> writes;
};
