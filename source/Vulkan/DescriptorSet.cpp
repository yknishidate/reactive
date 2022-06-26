#include <filesystem>
#include <regex>
#include "Context.hpp"
#include "Pipeline.hpp"
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/ResourceLimits.h>
#include <spirv_glsl.hpp>
#include <spdlog/spdlog.h>
#include "Image.hpp"
#include "DescriptorSet.hpp"
#include "Compiler.hpp"

namespace
{
    void UpdateBindingMap(std::unordered_map<std::string, vk::DescriptorSetLayoutBinding>& map,
                          spirv_cross::Resource& resource, spirv_cross::CompilerGLSL& glsl,
                          vk::ShaderStageFlags stage, vk::DescriptorType type)
    {
        if (map.contains(resource.name)) {
            auto& binding = map[resource.name];
            if (binding.binding != glsl.get_decoration(resource.id, spv::DecorationBinding)) {
                throw std::runtime_error("binding does not match.");
            }
            binding.stageFlags |= stage;
        } else {
            vk::DescriptorSetLayoutBinding binding;
            binding.binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
            binding.descriptorType = type;
            binding.descriptorCount = 1;
            binding.stageFlags = stage;
            map[resource.name] = binding;
        }
    }

    void AddBindingMap(const std::vector<uint32_t>& spvShader,
                       std::unordered_map<std::string, vk::DescriptorSetLayoutBinding>& map,
                       vk::ShaderStageFlags stage)
    {
        spirv_cross::CompilerGLSL glsl{ spvShader };
        spirv_cross::ShaderResources resources = glsl.get_shader_resources();

        for (auto&& resource : resources.uniform_buffers) {
            UpdateBindingMap(map, resource, glsl, stage, vk::DescriptorType::eUniformBuffer);
        }
        for (auto&& resource : resources.acceleration_structures) {
            UpdateBindingMap(map, resource, glsl, stage, vk::DescriptorType::eAccelerationStructureKHR);
        }
        for (auto&& resource : resources.storage_buffers) {
            UpdateBindingMap(map, resource, glsl, stage, vk::DescriptorType::eStorageBuffer);
        }
        for (auto&& resource : resources.storage_images) {
            UpdateBindingMap(map, resource, glsl, stage, vk::DescriptorType::eStorageImage);
        }
        for (auto&& resource : resources.sampled_images) {
            UpdateBindingMap(map, resource, glsl, stage, vk::DescriptorType::eCombinedImageSampler);
        }
    }

    vk::UniqueShaderModule CreateShaderModule(const std::vector<uint32_t>& code)
    {
        vk::ShaderModuleCreateInfo createInfo;
        createInfo.setCode(code);
        return Context::GetDevice().createShaderModuleUnique(createInfo);
    }

    std::vector<vk::DescriptorSetLayoutBinding> GetBindings(std::unordered_map<std::string, vk::DescriptorSetLayoutBinding>& map)
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        for (auto&& [name, binding] : map) {
            bindings.push_back(binding);
        }
        return bindings;
    }

    vk::UniqueDescriptorSetLayout CreateDescSetLayout(const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
    {
        vk::DescriptorSetLayoutCreateInfo createInfo;
        createInfo.setBindings(bindings);
        return Context::GetDevice().createDescriptorSetLayoutUnique(createInfo);
    }

    vk::UniqueDescriptorSet AllocateDescSet(vk::DescriptorSetLayout descSetLayout)
    {
        vk::DescriptorSetAllocateInfo allocateInfo;
        allocateInfo.setDescriptorPool(Context::GetDescriptorPool());
        allocateInfo.setSetLayouts(descSetLayout);
        std::vector descSets = Context::GetDevice().allocateDescriptorSetsUnique(allocateInfo);
        return std::move(descSets.front());
    }

    vk::WriteDescriptorSet MakeWrite(vk::DescriptorSetLayoutBinding binding)
    {
        vk::WriteDescriptorSet write;
        write.setDescriptorType(binding.descriptorType);
        write.setDescriptorCount(binding.descriptorCount);
        write.setDstBinding(binding.binding);
        return write;
    }

    void UpdateDescSet(vk::DescriptorSet descSet, std::vector<vk::WriteDescriptorSet>& writes)
    {
        for (auto&& write : writes) {
            write.setDstSet(descSet);
        }
        Context::GetDevice().updateDescriptorSets(writes, nullptr);
    }
} // namespace

void DescriptorSet::Allocate(const std::string& shaderPath)
{
    std::vector spirvCode = Compiler::CompileToSPV(shaderPath);
    AddBindingMap(spirvCode, bindingMap, vk::ShaderStageFlagBits::eCompute);
    std::vector bindings = GetBindings(bindingMap);
    descSetLayout = CreateDescSetLayout(bindings);
    descSet = AllocateDescSet(*descSetLayout);
}

void DescriptorSet::Update()
{
    std::vector<vk::WriteDescriptorSet> _writes(writes.size());
    for (int i = 0; i < writes.size(); i++) {
        _writes[i] = writes[i].Get();
    }
    // TODO
    //std::transform (writes.begin(), writes.end(), _writes.begin(), [](auto&& write) {
    //    return MakeWrite(write.first);
    //});

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
    if (images.size() == 0) {
        return;
    }
    std::vector<vk::DescriptorImageInfo> infos;
    for (auto&& image : images) {
        vk::DescriptorImageInfo info;
        info.setImageView(image.GetView());
        info.setSampler(image.GetSampler());
        info.setImageLayout(vk::ImageLayout::eGeneral);
        infos.push_back(info);
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
