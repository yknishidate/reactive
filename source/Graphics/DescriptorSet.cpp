#include <regex>
#include "Graphics.hpp"
#include "DescriptorSet.hpp"
#include "Compiler/Compiler.hpp"

namespace
{
    vk::UniqueDescriptorSetLayout CreateDescSetLayout(const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
    {
        return Graphics::GetDevice().createDescriptorSetLayoutUnique(
            vk::DescriptorSetLayoutCreateInfo()
            .setBindings(bindings)
        );
    }

    vk::UniqueDescriptorSet AllocateDescSet(vk::DescriptorSetLayout descSetLayout)
    {
        std::vector descSets = Graphics::GetDevice().allocateDescriptorSetsUnique(
            vk::DescriptorSetAllocateInfo()
            .setDescriptorPool(Graphics::GetDescriptorPool())
            .setSetLayouts(descSetLayout)
        );
        return std::move(descSets.front());
    }

    void UpdateDescSet(vk::DescriptorSet descSet, std::vector<vk::WriteDescriptorSet>& writes)
    {
        for (auto&& write : writes) {
            write.setDstSet(descSet);
        }
        Graphics::GetDevice().updateDescriptorSets(writes, nullptr);
    }
} // namespace

void DescriptorSet::Setup()
{
    descSetLayout = CreateDescSetLayout(GetBindings());
    descSet = AllocateDescSet(*descSetLayout);
    Update();
}

void DescriptorSet::Update()
{
    std::vector<vk::WriteDescriptorSet> _writes;
    for (auto&& write : writes) {
        _writes.push_back(write.Get());
    }
    UpdateDescSet(*descSet, _writes);
}

void DescriptorSet::Register(const std::string& name, const std::vector<Image>& images)
{
    std::vector<vk::DescriptorImageInfo> infos;
    infos.reserve(images.size());
    for (auto&& image : images) {
        infos.push_back(image.GetInfo());
    }

    bindingMap[name].descriptorCount = images.size();

    writes.emplace_back(bindingMap[name], infos);
}

void DescriptorSet::Register(const std::string& name, const Buffer& buffer)
{
    bindingMap[name].descriptorCount = 1;
    writes.emplace_back(bindingMap[name], buffer.GetInfo());
}

void DescriptorSet::Register(const std::string& name, const Image& image)
{
    bindingMap[name].descriptorCount = 1;
    writes.emplace_back(bindingMap[name], image.GetInfo());
}

void DescriptorSet::Register(const std::string& name, const TopAccel& accel)
{
    bindingMap[name].descriptorCount = 1;
    writes.emplace_back(bindingMap[name], accel.GetInfo());
}

void DescriptorSet::AddBindingMap(const std::vector<uint32_t>& spvShader, vk::ShaderStageFlags stage)
{
    spirv_cross::CompilerGLSL glsl{ spvShader };
    spirv_cross::ShaderResources resources = glsl.get_shader_resources();

    for (auto&& resource : resources.uniform_buffers) {
        UpdateBindingMap(resource, glsl, stage, vk::DescriptorType::eUniformBuffer);
    }
    for (auto&& resource : resources.acceleration_structures) {
        UpdateBindingMap(resource, glsl, stage, vk::DescriptorType::eAccelerationStructureKHR);
    }
    for (auto&& resource : resources.storage_buffers) {
        UpdateBindingMap(resource, glsl, stage, vk::DescriptorType::eStorageBuffer);
    }
    for (auto&& resource : resources.storage_images) {
        UpdateBindingMap(resource, glsl, stage, vk::DescriptorType::eStorageImage);
    }
    for (auto&& resource : resources.sampled_images) {
        UpdateBindingMap(resource, glsl, stage, vk::DescriptorType::eCombinedImageSampler);
    }
}

std::vector<vk::DescriptorSetLayoutBinding> DescriptorSet::GetBindings() const
{
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    for (auto&& [name, binding] : bindingMap) {
        bindings.push_back(binding);
    }
    return bindings;
}

vk::UniquePipelineLayout DescriptorSet::CreatePipelineLayout(size_t pushSize, vk::ShaderStageFlags shaderStage) const
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

    return Graphics::GetDevice().createPipelineLayoutUnique(layoutInfo);
}

void DescriptorSet::Bind(vk::CommandBuffer commandBuffer, vk::PipelineBindPoint bindPoint, vk::PipelineLayout pipelineLayout)
{
    commandBuffer.bindDescriptorSets(bindPoint, pipelineLayout, 0, *descSet, nullptr);
}

void DescriptorSet::UpdateBindingMap(spirv_cross::Resource& resource, spirv_cross::CompilerGLSL& glsl,
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
