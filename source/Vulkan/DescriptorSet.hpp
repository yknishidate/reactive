#pragma once
#include <unordered_map>

#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/ResourceLimits.h>
#include <spirv_glsl.hpp>

#include "Buffer.hpp"
#include "Image.hpp"
#include "WriteDescriptorSet.hpp"

class DescriptorSet
{
public:
    void SetupLayout();
    void Allocate();
    void Update();

    void Register(const std::string& name, vk::Buffer buffer, size_t size);
    void Register(const std::string& name, vk::ImageView view, vk::Sampler sampler);
    void Register(const std::string& name, const std::vector<Image>& images);
    void Register(const std::string& name, const Buffer& buffer);
    void Register(const std::string& name, const Image& image);
    void Register(const std::string& name, const vk::AccelerationStructureKHR& accel);

    void AddBindingMap(const std::vector<uint32_t>& spvShader, vk::ShaderStageFlags stage);

    auto GetLayout() const { return *descSetLayout; }
    auto Get() const { return *descSet; }

    std::vector<vk::DescriptorSetLayoutBinding> GetBindings() const;

private:
    void UpdateBindingMap(spirv_cross::Resource& resource, spirv_cross::CompilerGLSL& glsl,
                          vk::ShaderStageFlags stage, vk::DescriptorType type);

    vk::UniqueDescriptorSet descSet;
    vk::UniqueDescriptorSetLayout descSetLayout;
    std::unordered_map<std::string, vk::DescriptorSetLayoutBinding> bindingMap;
    std::vector<WriteDescriptorSet> writes;
};
