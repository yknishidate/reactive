#include "DescriptorSet.hpp"
#include <regex>
#include "Compiler/Compiler.hpp"

WriteDescriptorSet::WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding,
                                       vk::DescriptorBufferInfo bufferInfo)
    : WriteDescriptorSet{binding} {
    bufferInfos = {bufferInfo};
    write.setBufferInfo(bufferInfos);
}

WriteDescriptorSet::WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding,
                                       std::vector<vk::DescriptorBufferInfo> infos)
    : WriteDescriptorSet{binding} {
    bufferInfos = {infos};
    write.setBufferInfo(bufferInfos);
}

WriteDescriptorSet::WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding,
                                       vk::DescriptorImageInfo imageInfo)
    : WriteDescriptorSet{binding} {
    imageInfos = {imageInfo};
    write.setImageInfo(imageInfos);
}

WriteDescriptorSet::WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding,
                                       std::vector<vk::DescriptorImageInfo> infos)
    : WriteDescriptorSet{binding} {
    imageInfos = {infos};
    write.setImageInfo(imageInfos);
}

WriteDescriptorSet::WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding,
                                       vk::WriteDescriptorSetAccelerationStructureKHR accelInfo)
    : WriteDescriptorSet(binding) {
    accelInfos = {accelInfo};
    write.setPNext(&accelInfos.front());
}

WriteDescriptorSet::WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding) {
    write.setDescriptorType(binding.descriptorType);
    write.setDescriptorCount(binding.descriptorCount);
    write.setDstBinding(binding.binding);
}

DescriptorSet::DescriptorSet(const Context* context, DescriptorSetCreateInfo createInfo)
    : context{context} {
    for (auto& shader : createInfo.shaders) {
        addResources(*shader);
    }
    for (auto& [name, buffer] : createInfo.buffers) {
        assert(bindingMap.contains(name));
        bindingMap[name].descriptorCount = 1;
        writes.emplace_back(bindingMap[name], buffer.getInfo());
    }
    for (auto& [name, image] : createInfo.images) {
        assert(bindingMap.contains(name));
        bindingMap[name].descriptorCount = 1;
        writes.emplace_back(bindingMap[name], image.getInfo());
    }
    for (auto& [name, accel] : createInfo.accels) {
        assert(bindingMap.contains(name));
        bindingMap[name].descriptorCount = 1;
        writes.emplace_back(bindingMap[name], accel.getInfo());
    }
    allocate();
    update();
}

void DescriptorSet::allocate() {
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.reserve(bindingMap.size());
    for (auto& [name, binding] : bindingMap) {
        bindings.push_back(binding);
    }

    descSetLayout = context->getDevice().createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo().setBindings(bindings));
    descSet = context->allocateDescriptorSet(*descSetLayout);
}

void DescriptorSet::update() {
    std::vector<vk::WriteDescriptorSet> _writes;
    _writes.reserve(writes.size());
    for (auto& write : writes) {
        _writes.push_back(write.get());
    }
    for (auto& write : _writes) {
        write.setDstSet(*descSet);
    }
    context->getDevice().updateDescriptorSets(_writes, nullptr);
}

// void DescriptorSet::record(const std::string& name, const std::vector<Image>& images) {
//     assert(bindingMap.contains(name));
//     std::vector<vk::DescriptorImageInfo> infos;
//     infos.reserve(images.size());
//     for (auto& image : images) {
//         infos.push_back(image.getInfo());
//     }
//
//     bindingMap[name].descriptorCount = images.size();
//     writes.emplace_back(bindingMap[name], infos);
// }

// void DescriptorSet::record(const std::string& name, const TopAccel& accel) {
//     assert(bindingMap.contains(name));
//     bindingMap[name].descriptorCount = 1;
//     writes.emplace_back(bindingMap[name], accel.getInfo());
// }

void DescriptorSet::addResources(const Shader& shader) {
    vk::ShaderStageFlags stage = shader.getStage();
    spirv_cross::CompilerGLSL glsl{shader.getSpvCode()};
    spirv_cross::ShaderResources resources = glsl.get_shader_resources();

    for (auto& resource : resources.uniform_buffers) {
        updateBindingMap(resource, glsl, stage, vk::DescriptorType::eUniformBuffer);
    }
    for (auto& resource : resources.acceleration_structures) {
        updateBindingMap(resource, glsl, stage, vk::DescriptorType::eAccelerationStructureKHR);
    }
    for (auto& resource : resources.storage_buffers) {
        updateBindingMap(resource, glsl, stage, vk::DescriptorType::eStorageBuffer);
    }
    for (auto& resource : resources.storage_images) {
        updateBindingMap(resource, glsl, stage, vk::DescriptorType::eStorageImage);
    }
    for (auto& resource : resources.sampled_images) {
        updateBindingMap(resource, glsl, stage, vk::DescriptorType::eCombinedImageSampler);
    }
}

void DescriptorSet::bind(vk::CommandBuffer commandBuffer,
                         vk::PipelineBindPoint bindPoint,
                         vk::PipelineLayout pipelineLayout) {
    if (!writes.empty()) {
        commandBuffer.bindDescriptorSets(bindPoint, pipelineLayout, 0, *descSet, nullptr);
    }
}

void DescriptorSet::updateBindingMap(const spirv_cross::Resource& resource,
                                     const spirv_cross::CompilerGLSL& glsl,
                                     vk::ShaderStageFlags stage,
                                     vk::DescriptorType type) {
    if (bindingMap.contains(resource.name)) {
        auto& binding = bindingMap[resource.name];
        if (binding.binding != glsl.get_decoration(resource.id, spv::DecorationBinding)) {
            throw std::runtime_error("binding does not match.");
        }
        binding.stageFlags |= stage;
    } else {
        bindingMap[resource.name] =
            vk::DescriptorSetLayoutBinding()
                .setBinding(glsl.get_decoration(resource.id, spv::DecorationBinding))
                .setDescriptorType(type)
                .setDescriptorCount(1)
                .setStageFlags(stage);
    }
}
