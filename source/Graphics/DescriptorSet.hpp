#pragma once
#include <unordered_map>
#include <spirv_glsl.hpp>
#include "Buffer.hpp"
#include "Image.hpp"
#include "Accel.hpp"

class WriteDescriptorSet
{
public:
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, vk::DescriptorBufferInfo bufferInfo);
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, std::vector<vk::DescriptorBufferInfo> infos);
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, vk::DescriptorImageInfo imageInfo);
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, std::vector<vk::DescriptorImageInfo> infos);
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, vk::WriteDescriptorSetAccelerationStructureKHR accelInfo);

    vk::WriteDescriptorSet get() const { return write; }

private:
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding);

    vk::WriteDescriptorSet write{};
    std::vector<vk::DescriptorImageInfo> imageInfos;
    std::vector<vk::DescriptorBufferInfo> bufferInfos;
    std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> accelInfos;
};

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
