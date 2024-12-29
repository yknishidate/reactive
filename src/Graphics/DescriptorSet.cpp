#include "reactive/Graphics/DescriptorSet.hpp"

#include <ranges>
#include <stdexcept>
#include <vector>

#include "reactive/common.hpp"
#include "reactive/Graphics/Accel.hpp"
#include "reactive/Graphics/Buffer.hpp"

namespace rv {

DescriptorSet::DescriptorSet(const Context& context, const DescriptorSetCreateInfo& createInfo)
    : m_context{&context} {
    // シェーダーリソースを追加
    for (const auto& shader : createInfo.shaders) {
        addResources(shader);
    }

    // 各リソースのディスクリプタ情報を設定
    for (const auto& [name, buffers] : createInfo.buffers) {
        RV_ASSERT(m_descriptors.contains(name), "Unknown m_buffer m_name. Resource m_name must match the m_name on the m_shader.");
        if (std::holds_alternative<uint32_t>(buffers)) {
            m_descriptors[name].binding.setDescriptorCount(std::get<uint32_t>(buffers));
        } else {
            set(name, std::get<ArrayProxy<BufferHandle>>(buffers));
        }
    }
    for (const auto& [name, images] : createInfo.images) {
        RV_ASSERT(m_descriptors.contains(name), "Unknown m_image m_name. Resource m_name must match the m_name on the m_shader.");
        if (std::holds_alternative<uint32_t>(images)) {
            m_descriptors[name].binding.setDescriptorCount(std::get<uint32_t>(images));
        } else {
            set(name, std::get<ArrayProxy<ImageHandle>>(images));
        }
    }
    for (const auto& [name, accels] : createInfo.accels) {
        RV_ASSERT(m_descriptors.contains(name), "Unknown m_accel m_name. Resource m_name must match the m_name on the m_shader.");
        if (std::holds_alternative<uint32_t>(accels)) {
            m_descriptors[name].binding.setDescriptorCount(std::get<uint32_t>(accels));
        } else {
            set(name, std::get<ArrayProxy<TopAccelHandle>>(accels));
        }
    }

    // ディスクリプタセットレイアウトを作成
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    for (const auto& descriptor : m_descriptors | std::views::values) {
        bindings.push_back(descriptor.binding);
    }

    vk::DescriptorSetLayoutCreateInfo layoutInfo({}, bindings);
    m_descSetLayout = m_context->getDevice().createDescriptorSetLayoutUnique(layoutInfo);

    // ディスクリプタセットを確保
    vk::DescriptorSetAllocateInfo allocInfo(m_context->getDescriptorPool(), *m_descSetLayout);
    m_descSet = std::move(m_context->getDevice().allocateDescriptorSetsUnique(allocInfo).front());
}

void DescriptorSet::update() {
    std::vector<vk::WriteDescriptorSet> descriptorWrites;

    for (const auto& [binding, infos] : m_descriptors | std::views::values) {
        if (std::holds_alternative<BufferInfos>(infos)) {
            const auto& bufferInfos = std::get<BufferInfos>(infos);
            vk::WriteDescriptorSet descriptorWrite;
            descriptorWrite.setBufferInfo(bufferInfos);
            descriptorWrite.setDescriptorCount(static_cast<uint32_t>(bufferInfos.size()));
            descriptorWrite.setDescriptorType(binding.descriptorType);
            descriptorWrite.setDstBinding(binding.binding);
            descriptorWrite.setDstSet(*m_descSet);
            descriptorWrites.push_back(descriptorWrite);
        } else if (std::holds_alternative<ImageInfos>(infos)) {
            const auto& imageInfos = std::get<ImageInfos>(infos);
            vk::WriteDescriptorSet descriptorWrite;
            descriptorWrite.setImageInfo(imageInfos);
            descriptorWrite.setDescriptorCount(static_cast<uint32_t>(imageInfos.size()));
            descriptorWrite.setDescriptorType(binding.descriptorType);
            descriptorWrite.setDstBinding(binding.binding);
            descriptorWrite.setDstSet(*m_descSet);
            descriptorWrites.push_back(descriptorWrite);
        } else if (std::holds_alternative<AccelInfos>(infos)) {
            const auto& accelInfos = std::get<AccelInfos>(infos);
            vk::WriteDescriptorSet descriptorWrite;
            descriptorWrite.setDescriptorCount(static_cast<uint32_t>(accelInfos.size()));
            descriptorWrite.setDescriptorType(binding.descriptorType);
            descriptorWrite.setDstBinding(binding.binding);
            descriptorWrite.setDstSet(*m_descSet);
            descriptorWrite.setPNext(accelInfos.data());
            descriptorWrites.push_back(descriptorWrite);
        }
    }

    m_context->getDevice().updateDescriptorSets(descriptorWrites, nullptr);
}

void DescriptorSet::set(const std::string& name, ArrayProxy<BufferHandle> buffers) {
    // バッファのディスクリプタ情報を設定
    std::vector<vk::DescriptorBufferInfo> bufferInfos;
    for (const auto& buffer : buffers) {
        bufferInfos.push_back(buffer->getInfo());
    }
    m_descriptors[name].binding.descriptorCount = buffers.size();
    m_descriptors[name].infos = bufferInfos;
}

void DescriptorSet::set(const std::string& name, ArrayProxy<ImageHandle> images) {
    // イメージのディスクリプタ情報を設定
    ImageInfos imageInfos;
    for (const auto& image : images) {
        imageInfos.push_back(image->getInfo());
    }
    m_descriptors[name].binding.descriptorCount = images.size();
    m_descriptors[name].infos = imageInfos;
}

void DescriptorSet::set(const std::string& name, ArrayProxy<TopAccelHandle> accels) {
    // アクセラレーション構造のディスクリプタ情報を設定
    AccelInfos accelInfos;
    for (const auto& accel : accels) {
        accelInfos.push_back(accel->getInfo());
    }
    m_descriptors[name].binding.descriptorCount = accels.size();
    m_descriptors[name].infos = accelInfos;
}

void DescriptorSet::addResources(ShaderHandle shader) {
    // シェーダーリソースを解析してディスクリプタ情報を更新
    vk::ShaderStageFlags stage = shader->getStage();
    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(shader->getSpvCodeSize(), shader->getSpvCodePtr(), &module);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to create SPIRV-Reflect m_shader module.");
    }

    uint32_t count = 0;
    result = spvReflectEnumerateDescriptorBindings(&module, &count, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        spvReflectDestroyShaderModule(&module);
        throw std::runtime_error("Failed to enumerate descriptor bindings.");
    }

    std::vector<SpvReflectDescriptorBinding*> bindings(count);
    result = spvReflectEnumerateDescriptorBindings(&module, &count, bindings.data());
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        spvReflectDestroyShaderModule(&module);
        throw std::runtime_error("Failed to enumerate descriptor bindings.");
    }

    for (const auto* binding : bindings) {
        updateBindingMap(binding, stage);
    }

    spvReflectDestroyShaderModule(&module);
}

void DescriptorSet::updateBindingMap(const SpvReflectDescriptorBinding* binding, vk::ShaderStageFlags stage) {
    const std::string& name = binding->name;
    if (m_descriptors.contains(name)) {
        auto& desc_binding = m_descriptors[name].binding;
        if (desc_binding.binding != binding->binding) {
            throw std::runtime_error("binding does not match.");
        }
        desc_binding.stageFlags |= stage;
    } else {
        m_descriptors[name] = {
            .binding = vk::DescriptorSetLayoutBinding()
                           .setBinding(binding->binding)
                           .setDescriptorType(static_cast<vk::DescriptorType>(binding->descriptor_type))
                           .setDescriptorCount(binding->count)
                           .setStageFlags(stage),
        };
    }
}
}  // namespace rv
