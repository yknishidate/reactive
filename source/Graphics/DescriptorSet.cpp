#include <regex>
#include "Context.hpp"
#include "DescriptorSet.hpp"
#include "Compiler/Compiler.hpp"

namespace
{
vk::UniqueDescriptorSetLayout CreateDescSetLayout(const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
{
    return Context::getDevice().createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo()
        .setBindings(bindings)
    );
}

vk::UniqueDescriptorSet AllocateDescSet(vk::DescriptorSetLayout descSetLayout)
{
    std::vector descSets = Context::getDevice().allocateDescriptorSetsUnique(
        vk::DescriptorSetAllocateInfo()
        .setDescriptorPool(Context::getDescriptorPool())
        .setSetLayouts(descSetLayout)
    );
    return std::move(descSets.front());
}

void UpdateDescSet(vk::DescriptorSet descSet, std::vector<vk::WriteDescriptorSet>& writes)
{
    for (auto&& write : writes) {
        write.setDstSet(descSet);
    }
    Context::getDevice().updateDescriptorSets(writes, nullptr);
}
} // namespace

void DescriptorSet::setup()
{
    descSetLayout = CreateDescSetLayout(getBindings());
    descSet = AllocateDescSet(*descSetLayout);
    update();
}

void DescriptorSet::update()
{
    std::vector<vk::WriteDescriptorSet> _writes;
    for (auto&& write : writes) {
        _writes.push_back(write.get());
    }
    UpdateDescSet(*descSet, _writes);
}

void DescriptorSet::record(const std::string& name, const std::vector<Image>& images)
{
    std::vector<vk::DescriptorImageInfo> infos;
    infos.reserve(images.size());
    for (auto&& image : images) {
        infos.push_back(image.getInfo());
    }

    bindingMap[name].descriptorCount = images.size();

    writes.emplace_back(bindingMap[name], infos);
}

void DescriptorSet::record(const std::string& name, const Buffer& buffer)
{
    bindingMap[name].descriptorCount = 1;
    writes.emplace_back(bindingMap[name], buffer.getInfo());
}

void DescriptorSet::record(const std::string& name, const Image& image)
{
    bindingMap[name].descriptorCount = 1;
    writes.emplace_back(bindingMap[name], image.getInfo());
}

void DescriptorSet::record(const std::string& name, const TopAccel& accel)
{
    bindingMap[name].descriptorCount = 1;
    writes.emplace_back(bindingMap[name], accel.getInfo());
}

void DescriptorSet::addBindingMap(const std::vector<uint32_t>& spvShader, vk::ShaderStageFlags stage)
{
    spirv_cross::CompilerGLSL glsl{ spvShader };
    spirv_cross::ShaderResources resources = glsl.get_shader_resources();

    for (auto&& resource : resources.uniform_buffers) {
        updateBindingMap(resource, glsl, stage, vk::DescriptorType::eUniformBuffer);
    }
    for (auto&& resource : resources.acceleration_structures) {
        updateBindingMap(resource, glsl, stage, vk::DescriptorType::eAccelerationStructureKHR);
    }
    for (auto&& resource : resources.storage_buffers) {
        updateBindingMap(resource, glsl, stage, vk::DescriptorType::eStorageBuffer);
    }
    for (auto&& resource : resources.storage_images) {
        updateBindingMap(resource, glsl, stage, vk::DescriptorType::eStorageImage);
    }
    for (auto&& resource : resources.sampled_images) {
        updateBindingMap(resource, glsl, stage, vk::DescriptorType::eCombinedImageSampler);
    }
}

std::vector<vk::DescriptorSetLayoutBinding> DescriptorSet::getBindings() const
{
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    for (auto&& [name, binding] : bindingMap) {
        bindings.push_back(binding);
    }
    return bindings;
}

vk::UniquePipelineLayout DescriptorSet::createPipelineLayout(size_t pushSize, vk::ShaderStageFlags shaderStage) const
{
    const auto pushRange = vk::PushConstantRange()
        .setOffset(0)
        .setSize(pushSize)
        .setStageFlags(shaderStage);

    auto layoutInfo = vk::PipelineLayoutCreateInfo()
        .setSetLayouts(*descSetLayout);

    if (pushSize) {
        layoutInfo.setPushConstantRanges(pushRange);
    }

    return Context::getDevice().createPipelineLayoutUnique(layoutInfo);
}

void DescriptorSet::bind(vk::CommandBuffer commandBuffer, vk::PipelineBindPoint bindPoint, vk::PipelineLayout pipelineLayout)
{
    commandBuffer.bindDescriptorSets(bindPoint, pipelineLayout, 0, *descSet, nullptr);
}

void DescriptorSet::updateBindingMap(spirv_cross::Resource& resource, spirv_cross::CompilerGLSL& glsl,
                                     vk::ShaderStageFlags stage, vk::DescriptorType type)
{
    if (bindingMap.contains(resource.name)) {
        auto& binding = bindingMap[resource.name];
        if (binding.binding != glsl.get_decoration(resource.id, spv::DecorationBinding)) {
            throw std::runtime_error("binding does not match.");
        }
        binding.stageFlags |= stage;
    } else {
        bindingMap[resource.name] = vk::DescriptorSetLayoutBinding()
            .setBinding(glsl.get_decoration(resource.id, spv::DecorationBinding))
            .setDescriptorType(type)
            .setDescriptorCount(1)
            .setStageFlags(stage);
    }
}
