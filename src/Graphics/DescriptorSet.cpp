#include "Graphics/DescriptorSet.hpp"

#include <ranges>
#include <stdexcept>
#include <vector>

#include "Graphics/Accel.hpp"
#include "Graphics/Buffer.hpp"

namespace rv {

DescriptorSet::DescriptorSet(const Context* context, DescriptorSetCreateInfo createInfo)
    : context(context) {
    // シェーダーリソースを追加
    for (const auto& shader : createInfo.shaders) {
        addResources(shader);
    }

    // 各リソースのディスクリプタ情報を設定
    for (const auto& [name, buffers] : createInfo.buffers) {
        set(name, buffers);
    }
    for (const auto& [name, images] : createInfo.images) {
        set(name, images);
    }
    for (const auto& [name, accels] : createInfo.accels) {
        set(name, accels);
    }

    // ディスクリプタセットレイアウトを作成
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    for (const auto& descriptor : descriptors | std::views::values) {
        bindings.push_back(descriptor.binding);
    }

    vk::DescriptorSetLayoutCreateInfo layoutInfo({}, bindings);
    descSetLayout = context->getDevice().createDescriptorSetLayoutUnique(layoutInfo);

    // ディスクリプタセットを確保
    vk::DescriptorSetAllocateInfo allocInfo(context->getDescriptorPool(), *descSetLayout);
    descSet = std::move(context->getDevice().allocateDescriptorSetsUnique(allocInfo).front());

    // 更新する
    update();
}

void DescriptorSet::update() {
    std::vector<vk::WriteDescriptorSet> descriptorWrites;

    for (const auto& [binding, infos] : descriptors | std::views::values) {
        if (std::holds_alternative<BufferInfos>(infos)) {
            const auto& bufferInfos = std::get<BufferInfos>(infos);
            vk::WriteDescriptorSet descriptorWrite;
            descriptorWrite.setBufferInfo(bufferInfos);
            descriptorWrite.setDescriptorCount(static_cast<uint32_t>(bufferInfos.size()));
            descriptorWrite.setDescriptorType(binding.descriptorType);
            descriptorWrite.setDstBinding(binding.binding);
            descriptorWrite.setDstSet(*descSet);
            descriptorWrites.push_back(descriptorWrite);
        } else if (std::holds_alternative<ImageInfos>(infos)) {
            const auto& imageInfos = std::get<ImageInfos>(infos);
            vk::WriteDescriptorSet descriptorWrite;
            descriptorWrite.setImageInfo(imageInfos);
            descriptorWrite.setDescriptorCount(static_cast<uint32_t>(imageInfos.size()));
            descriptorWrite.setDescriptorType(binding.descriptorType);
            descriptorWrite.setDstBinding(binding.binding);
            descriptorWrite.setDstSet(*descSet);
            descriptorWrites.push_back(descriptorWrite);
        } else if (std::holds_alternative<AccelInfos>(infos)) {
            const auto& accelInfos = std::get<AccelInfos>(infos);
            vk::WriteDescriptorSet descriptorWrite;
            descriptorWrite.setDescriptorCount(static_cast<uint32_t>(accelInfos.size()));
            descriptorWrite.setDescriptorType(binding.descriptorType);
            descriptorWrite.setDstBinding(binding.binding);
            descriptorWrite.setDstSet(*descSet);
            descriptorWrite.setPNext(&accelInfos);
            descriptorWrites.push_back(descriptorWrite);
        }
    }

    context->getDevice().updateDescriptorSets(descriptorWrites, nullptr);
}

void DescriptorSet::set(const std::string& name, ArrayProxy<BufferHandle> buffers) {
    // バッファのディスクリプタ情報を設定
    std::vector<vk::DescriptorBufferInfo> bufferInfos;
    for (const auto& buffer : buffers) {
        bufferInfos.push_back(buffer->getInfo());
    }
    descriptors[name].infos = bufferInfos;
}

void DescriptorSet::set(const std::string& name, ArrayProxy<ImageHandle> images) {
    // イメージのディスクリプタ情報を設定
    ImageInfos imageInfos;
    for (const auto& image : images) {
        imageInfos.push_back(image->getInfo());
    }
    descriptors[name].infos = imageInfos;
}

void DescriptorSet::set(const std::string& name, ArrayProxy<TopAccelHandle> accels) {
    // アクセラレーション構造のディスクリプタ情報を設定
    AccelInfos accelInfos;
    for (const auto& accel : accels) {
        accelInfos.push_back(accel->getInfo());
    }
    descriptors[name].infos = accelInfos;
}

void DescriptorSet::addResources(ShaderHandle shader) {
    // シェーダーリソースを解析してディスクリプタ情報を更新
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

void DescriptorSet::updateBindingMap(const spirv_cross::Resource& resource,
                                     const spirv_cross::CompilerGLSL& glsl,
                                     vk::ShaderStageFlags stage,
                                     vk::DescriptorType type) {
    if (descriptors.contains(resource.name)) {
        auto& binding = descriptors[resource.name].binding;
        if (binding.binding != glsl.get_decoration(resource.id, spv::DecorationBinding)) {
            throw std::runtime_error("binding does not match.");
        }
        binding.stageFlags |= stage;
    } else {
        descriptors[resource.name] = {
            .binding = vk::DescriptorSetLayoutBinding()
                           .setBinding(glsl.get_decoration(resource.id, spv::DecorationBinding))
                           .setDescriptorType(type)
                           .setDescriptorCount(1)
                           .setStageFlags(stage),
        };
    }
}
}  // namespace rv
