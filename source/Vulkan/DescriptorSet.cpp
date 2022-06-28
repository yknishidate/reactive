#include <filesystem>
#include <regex>
#include <spdlog/spdlog.h>
#include "Context.hpp"
#include "Pipeline.hpp"
#include "DescriptorSet.hpp"
#include "Compiler.hpp"

namespace
{
    vk::UniqueShaderModule CreateShaderModule(const std::vector<uint32_t>& code)
    {
        return Context::GetDevice().createShaderModuleUnique(
            vk::ShaderModuleCreateInfo()
            .setCode(code)
        );
    }

    vk::UniqueDescriptorSetLayout CreateDescSetLayout(const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
    {
        return Context::GetDevice().createDescriptorSetLayoutUnique(
            vk::DescriptorSetLayoutCreateInfo()
            .setBindings(bindings)
        );
    }

    vk::UniqueDescriptorSet AllocateDescSet(vk::DescriptorSetLayout descSetLayout)
    {
        std::vector descSets = Context::GetDevice().allocateDescriptorSetsUnique(
            vk::DescriptorSetAllocateInfo()
            .setDescriptorPool(Context::GetDescriptorPool())
            .setSetLayouts(descSetLayout)
        );
        return std::move(descSets.front());
    }

    void UpdateDescSet(vk::DescriptorSet descSet, std::vector<vk::WriteDescriptorSet>& writes)
    {
        for (auto&& write : writes) {
            write.setDstSet(descSet);
        }
        Context::GetDevice().updateDescriptorSets(writes, nullptr);
    }
} // namespace

void DescriptorSet::SetupLayout()
{
    descSetLayout = CreateDescSetLayout(GetBindings());
}

void DescriptorSet::Allocate()
{
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

void DescriptorSet::Register(const std::string& name, vk::Buffer buffer, size_t size)
{
    vk::DescriptorBufferInfo bufferInfo{ buffer, 0, size };
    bindingMap[name].descriptorCount = 1;
    writes.emplace_back(bindingMap[name], bufferInfo);
}

void DescriptorSet::Register(const std::string& name, vk::ImageView view, vk::Sampler sampler)
{
    vk::DescriptorImageInfo imageInfo;
    imageInfo.setImageView(view);
    imageInfo.setSampler(sampler);
    imageInfo.setImageLayout(vk::ImageLayout::eGeneral);
    bindingMap[name].descriptorCount = 1;

    writes.emplace_back(bindingMap[name], imageInfo);
}

void DescriptorSet::Register(const std::string& name, const std::vector<Image>& images)
{
    std::vector<vk::DescriptorImageInfo> infos;
    for (auto&& image : images) {
        infos.push_back(
            vk::DescriptorImageInfo()
            .setImageView(image.GetView())
            .setSampler(image.GetSampler())
            .setImageLayout(vk::ImageLayout::eGeneral)
        );
    }

    bindingMap[name].descriptorCount = images.size();

    writes.emplace_back(bindingMap[name], infos);
}

void DescriptorSet::Register(const std::string& name, const Buffer& buffer)
{
    Register(name, buffer.GetBuffer(), buffer.GetSize());
}

void DescriptorSet::Register(const std::string& name, const Image& image)
{
    Register(name, image.GetView(), image.GetSampler());
}

void DescriptorSet::Register(const std::string& name, const vk::AccelerationStructureKHR& accel)
{
    vk::WriteDescriptorSetAccelerationStructureKHR accelInfo{ accel };
    bindingMap[name].descriptorCount = 1;

    writes.emplace_back(bindingMap[name], accelInfo);
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

    return Context::GetDevice().createPipelineLayoutUnique(layoutInfo);
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
