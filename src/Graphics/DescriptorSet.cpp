#include "Graphics/DescriptorSet.hpp"

#include <ranges>
#include <regex>

#include "Compiler/Compiler.hpp"
#include "common.hpp"

namespace rv {
WriteDescriptorSet::WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding,
                                       ArrayProxy<vk::DescriptorBufferInfo> infos)
    : WriteDescriptorSet{binding} {
    bufferInfos.reserve(infos.size());
    for (auto& info : infos) {
        bufferInfos.push_back(info);
    }
    write.setBufferInfo(bufferInfos);
}

WriteDescriptorSet::WriteDescriptorSet(vk::DescriptorSetLayoutBinding binding,
                                       ArrayProxy<vk::DescriptorImageInfo> infos)
    : WriteDescriptorSet{binding} {
    imageInfos.reserve(infos.size());
    for (auto& info : infos) {
        imageInfos.push_back(info);
    }
    write.setImageInfo(imageInfos);
}

WriteDescriptorSet::WriteDescriptorSet(
    vk::DescriptorSetLayoutBinding binding,
    ArrayProxy<vk::WriteDescriptorSetAccelerationStructureKHR> infos)
    : WriteDescriptorSet(binding) {
    accelInfos.reserve(infos.size());
    for (auto& info : infos) {
        accelInfos.push_back(info);
    }
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
            infos.push_back(b->getInfo());
        }

        writes[name] = {bindingMap[name], infos};
    }
    for (auto& [name, image] : createInfo.images) {
        RV_ASSERT(bindingMap.contains(name), "bindingMap does not contain key: {}", name);
        bindingMap[name].descriptorCount = image.size();

        std::vector<vk::DescriptorImageInfo> infos;
        for (auto& i : image) {
            infos.push_back(i->getInfo());
        }

        writes[name] = {bindingMap[name], infos};
    }
    for (auto& [name, accel] : createInfo.accels) {
        RV_ASSERT(bindingMap.contains(name), "bindingMap does not contain key: {}", name);
        bindingMap[name].descriptorCount = accel.size();

        std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> infos;
        for (auto& a : accel) {
            infos.push_back(a->getInfo());
        }

        writes[name] = {bindingMap[name], infos};
    }
    allocate();
    update();
}

void DescriptorSet::allocate() {
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.reserve(bindingMap.size());
    for (auto& binding : bindingMap | std::views::values) {
        bindings.push_back(binding);
    }

    vk::DescriptorSetLayoutCreateInfo layoutInfo;
    layoutInfo.setBindings(bindings);
    descSetLayout = context->getDevice().createDescriptorSetLayoutUnique(layoutInfo);

    vk::DescriptorSetAllocateInfo descSetInfo;
    descSetInfo.setDescriptorPool(context->getDescriptorPool());
    descSetInfo.setSetLayouts(*descSetLayout);
    descSet = std::move(context->getDevice().allocateDescriptorSetsUnique(descSetInfo).front());
}

void DescriptorSet::update() {
    std::vector<vk::WriteDescriptorSet> _writes;
    _writes.reserve(writes.size());
    for (auto& write : writes | std::views::values) {
        _writes.push_back(write.get());
        _writes.back().setDstSet(*descSet);
    }
    context->getDevice().updateDescriptorSets(_writes, nullptr);
}

void DescriptorSet::addResources(ShaderHandle shader) {
    vk::ShaderStageFlags stage = shader->getStage();
    spirv_cross::CompilerGLSL glsl{shader->getSpvCode()};
    const spirv_cross::ShaderResources& resources = glsl.get_shader_resources();

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

void DescriptorSet::set(const std::string& name, ArrayProxy<BufferHandle> buffers) {
    RV_ASSERT(bindingMap.contains(name), "bindingMap does not contain key: {}", name);
    std::vector<vk::DescriptorBufferInfo> infos;
    for (auto& buffer : buffers) {
        infos.push_back(buffer->getInfo());
    }
    writes[name] = {bindingMap[name], infos};
}

void DescriptorSet::set(const std::string& name, ArrayProxy<ImageHandle> images) {
    RV_ASSERT(bindingMap.contains(name), "bindingMap does not contain key: {}", name);
    std::vector<vk::DescriptorImageInfo> infos;
    for (auto& image : images) {
        infos.push_back(image->getInfo());
    }
    writes[name] = {bindingMap[name], infos};
}

void DescriptorSet::set(const std::string& name, ArrayProxy<TopAccelHandle> accels) {
    RV_ASSERT(bindingMap.contains(name), "bindingMap does not contain key: {}", name);
    std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> infos;
    for (auto& accel : accels) {
        infos.push_back(accel->getInfo());
    }
    writes[name] = {bindingMap[name], infos};
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
}  // namespace rv
