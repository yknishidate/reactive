#pragma once
#include <vulkan/vulkan.hpp>

class WriteDescriptorSet
{
public:
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, vk::DescriptorBufferInfo bufferInfo);

    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, std::vector<vk::DescriptorBufferInfo> infos);

    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, vk::DescriptorImageInfo imageInfo);

    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, std::vector<vk::DescriptorImageInfo> infos);

    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, vk::WriteDescriptorSetAccelerationStructureKHR accelInfo);

    vk::WriteDescriptorSet Get() const { return write; }

private:
    WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding);

    vk::WriteDescriptorSet write{};
    std::vector<vk::DescriptorImageInfo> imageInfos;
    std::vector<vk::DescriptorBufferInfo> bufferInfos;
    std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> accelInfos;
};
