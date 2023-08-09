#include "Graphics/DescriptorSet.hpp"

#include <regex>

#include "Compiler/Compiler.hpp"
#include "common.hpp"

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

WriteDescriptorSet::WriteDescriptorSet(
    vk::DescriptorSetLayoutBinding binding,
    std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> infos)
    : WriteDescriptorSet(binding) {
    accelInfos = {infos};
    write.setPNext(accelInfos.data());
}

WriteDescriptorSet::WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding) {
    write.setDescriptorType(binding.descriptorType);
    write.setDescriptorCount(binding.descriptorCount);
    write.setDstBinding(binding.binding);
}

DescriptorSet::DescriptorSet(const Context* context, DescriptorSetCreateInfo createInfo)
    : context{context} {
    for (auto& shader : createInfo.shaders) {
        addResources(shader);
    }
    for (auto& [name, buffer] : createInfo.buffers) {
        RV_ASSERT(bindingMap.contains(name), "bindingMap does not contain key: {}", name);
        bindingMap[name].descriptorCount = buffer.size();

        std::vector<vk::DescriptorBufferInfo> infos;
        for (auto& b : buffer) {
            infos.push_back(b.getInfo());
        }

        writes.emplace_back(bindingMap[name], infos);
    }
    for (auto& [name, image] : createInfo.images) {
        RV_ASSERT(bindingMap.contains(name), "bindingMap does not contain key: {}", name);
        bindingMap[name].descriptorCount = image.size();

        std::vector<vk::DescriptorImageInfo> infos;
        for (auto& i : image) {
            infos.push_back(i.getInfo());
        }

        writes.emplace_back(bindingMap[name], infos);
    }
    for (auto& [name, accel] : createInfo.accels) {
        RV_ASSERT(bindingMap.contains(name), "bindingMap does not contain key: {}", name);
        bindingMap[name].descriptorCount = accel.size();

        std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> infos;
        for (auto& a : accel) {
            infos.push_back(a.getInfo());
        }

        writes.emplace_back(bindingMap[name], infos);
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
