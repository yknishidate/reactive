#pragma once
#include <unordered_map>

#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/ResourceLimits.h>
#include <spirv_glsl.hpp>

#include "Buffer.hpp"
#include "Image.hpp"

class WriteDescriptorSet
{
public:
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, vk::DescriptorBufferInfo bufferInfo)
        : WriteDescriptorSet{ binding }
    {
        bufferInfos = { bufferInfo };
        write.setBufferInfo(bufferInfos);
    }

    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, std::vector<vk::DescriptorBufferInfo> infos)
        : WriteDescriptorSet{ binding }
    {
        bufferInfos = { infos };
        write.setBufferInfo(bufferInfos);
    }

    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, vk::DescriptorImageInfo imageInfo)
        : WriteDescriptorSet{ binding }
    {
        imageInfos = { imageInfo };
        write.setImageInfo(imageInfos);
    }

    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, std::vector<vk::DescriptorImageInfo> infos)
        : WriteDescriptorSet{ binding }
    {
        imageInfos = { infos };
        write.setImageInfo(imageInfos);
    }

    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, vk::WriteDescriptorSetAccelerationStructureKHR accelInfo)
        : WriteDescriptorSet(binding)
    {
        accelInfos = { accelInfo };
        write.setPNext(&accelInfos.front());
    }

    vk::WriteDescriptorSet Get() const { return write; }

private:
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding)
    {
        write.setDescriptorType(binding.descriptorType);
        write.setDescriptorCount(binding.descriptorCount);
        write.setDstBinding(binding.binding);
    }

    vk::WriteDescriptorSet write{};
    std::vector<vk::DescriptorImageInfo> imageInfos;
    std::vector<vk::DescriptorBufferInfo> bufferInfos;
    std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> accelInfos;
};

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
