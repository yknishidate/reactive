#include "WriteDescriptorSet.hpp"

WriteDescriptorSet::WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, vk::DescriptorBufferInfo bufferInfo)
    : WriteDescriptorSet{ binding }
{
    bufferInfos = { bufferInfo };
    write.setBufferInfo(bufferInfos);
}

WriteDescriptorSet::WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, std::vector<vk::DescriptorBufferInfo> infos)
    : WriteDescriptorSet{ binding }
{
    bufferInfos = { infos };
    write.setBufferInfo(bufferInfos);
}

WriteDescriptorSet::WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, vk::DescriptorImageInfo imageInfo)
    : WriteDescriptorSet{ binding }
{
    imageInfos = { imageInfo };
    write.setImageInfo(imageInfos);
}

WriteDescriptorSet::WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, std::vector<vk::DescriptorImageInfo> infos)
    : WriteDescriptorSet{ binding }
{
    imageInfos = { infos };
    write.setImageInfo(imageInfos);
}

WriteDescriptorSet::WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding, vk::WriteDescriptorSetAccelerationStructureKHR accelInfo)
    : WriteDescriptorSet(binding)
{
    accelInfos = { accelInfo };
    write.setPNext(&accelInfos.front());
}

WriteDescriptorSet::WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding)
{
    write.setDescriptorType(binding.descriptorType);
    write.setDescriptorCount(binding.descriptorCount);
    write.setDstBinding(binding.binding);
}