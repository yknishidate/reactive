#pragma once
#include <unordered_map>
#include "Buffer.hpp"
#include "Image.hpp"

class WriteDescriptorSet
{
public:
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, vk::DescriptorBufferInfo bufferInfo)
        : bufferInfos{ bufferInfo }
    {
        write.setDescriptorType(binding.descriptorType);
        write.setDescriptorCount(binding.descriptorCount);
        write.setDstBinding(binding.binding);
        write.setBufferInfo(bufferInfos);
    }

    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, std::vector<vk::DescriptorBufferInfo> infos)
        : bufferInfos{ infos }
    {
        write.setDescriptorType(binding.descriptorType);
        write.setDescriptorCount(binding.descriptorCount);
        write.setDstBinding(binding.binding);
        write.setBufferInfo(bufferInfos);
    }

    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, vk::DescriptorImageInfo imageInfo)
        : imageInfos{ imageInfo }
    {
        write.setDescriptorType(binding.descriptorType);
        write.setDescriptorCount(binding.descriptorCount);
        write.setDstBinding(binding.binding);
        write.setImageInfo(imageInfos);
    }

    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, std::vector<vk::DescriptorImageInfo> infos)
        : imageInfos{ infos }
    {
        write.setDescriptorType(binding.descriptorType);
        write.setDescriptorCount(binding.descriptorCount);
        write.setDstBinding(binding.binding);
        write.setImageInfo(imageInfos);
    }

    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, vk::WriteDescriptorSetAccelerationStructureKHR accelInfo)
        : accelInfos{ accelInfo }
    {
        write.setDescriptorType(binding.descriptorType);
        write.setDescriptorCount(binding.descriptorCount);
        write.setDstBinding(binding.binding);
        write.setPNext(&accelInfos.front());
    }

    vk::WriteDescriptorSet Get() const { return write; }

private:
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
    vk::UniqueDescriptorSet descSet;
    vk::UniqueDescriptorSetLayout descSetLayout;
    std::unordered_map<std::string, vk::DescriptorSetLayoutBinding> bindingMap;
    std::vector<WriteDescriptorSet> writes;
};
