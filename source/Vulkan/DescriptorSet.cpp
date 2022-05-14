#include <filesystem>
#include <regex>
#include "Vulkan.hpp"
#include "Pipeline.hpp"
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/ResourceLimits.h>
#include <spirv_glsl.hpp>
#include <spdlog/spdlog.h>
#include "Image.hpp"
#include "DescriptorSet.hpp"

namespace
{
    std::string ReadFile(const std::string& path)
    {
        std::ifstream input_file{ path };
        if (!input_file.is_open()) {
            throw std::runtime_error("Failed to open file: " + path);
        }
        return { (std::istreambuf_iterator<char>{input_file}), std::istreambuf_iterator<char>{} };
    }

    EShLanguage GetShaderStage(const std::string& filepath)
    {
        if (filepath.ends_with("vert")) return EShLangVertex;
        else if (filepath.ends_with("frag")) return EShLangFragment;
        else if (filepath.ends_with("comp")) return EShLangCompute;
        else if (filepath.ends_with("rgen")) return EShLangRayGenNV;
        else if (filepath.ends_with("rmiss")) return EShLangMissNV;
        else if (filepath.ends_with("rchit")) return EShLangClosestHitNV;
        else if (filepath.ends_with("rahit")) return EShLangAnyHitNV;
        else assert(false && "Unknown shader stage");
    }

    std::string Include(const std::string& filepath, const std::string& sourceText)
    {
        using std::filesystem::path;
        path dir = path{ filepath }.parent_path();

        std::string included = sourceText;
        std::regex regex{ "#include \"(.*)\"" };
        std::smatch results;
        while (std::regex_search(included, results, regex)) {
            path includePath = dir / results[1].str();
            std::string includeText = ReadFile(includePath.string());
            included.replace(results.position(), results.length(), includeText);
        }
        return included;
    }

    std::vector<uint32_t> CompileToSPV(const std::string& filepath)
    {
        spdlog::info("  Compile shader: {}", filepath);
        std::string glslShader = ReadFile(filepath);
        EShLanguage stage = GetShaderStage(filepath);
        std::string included = Include(filepath, glslShader);

        glslang::InitializeProcess();

        const char* shaderStrings = included.data();
        glslang::TShader shader(stage);
        shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_4);
        shader.setStrings(&shaderStrings, 1);

        EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
        if (!shader.parse(&glslang::DefaultTBuiltInResource, 100, false, messages)) {
            throw std::runtime_error("Failed to parse: " + glslShader + "\n" + shader.getInfoLog());
        }

        glslang::TProgram program;
        program.addShader(&shader);
        if (!program.link(messages)) {
            throw std::runtime_error("Failed to link: " + glslShader + "\n" + shader.getInfoLog());
        }

        std::vector<uint32_t> spvShader;
        glslang::GlslangToSpv(*program.getIntermediate(stage), spvShader);
        glslang::FinalizeProcess();
        return spvShader;
    }

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
        return Vulkan::GetDevice().createShaderModuleUnique(createInfo);
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
        return Vulkan::GetDevice().createDescriptorSetLayoutUnique(createInfo);
    }

    vk::UniqueDescriptorSet AllocateDescSet(vk::DescriptorSetLayout descSetLayout)
    {
        vk::DescriptorSetAllocateInfo allocateInfo;
        allocateInfo.setDescriptorPool(Vulkan::GetDescriptorPool());
        allocateInfo.setSetLayouts(descSetLayout);
        std::vector descSets = Vulkan::GetDevice().allocateDescriptorSetsUnique(allocateInfo);
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
        Vulkan::GetDevice().updateDescriptorSets(writes, nullptr);
    }
} // namespace

void DescriptorSet::Allocate(const std::string& shaderPath)
{
    std::vector spirvCode = CompileToSPV(shaderPath);
    AddBindingMap(spirvCode, bindingMap, vk::ShaderStageFlagBits::eCompute);
    std::vector bindings = GetBindings(bindingMap);
    descSetLayout = CreateDescSetLayout(bindings);
    descSet = AllocateDescSet(*descSetLayout);
}

void DescriptorSet::Update()
{
    UpdateDescSet(*descSet, writes);
}

void DescriptorSet::Register(const std::string& name, vk::Buffer buffer, size_t size)
{
    vk::DescriptorBufferInfo bufferInfo{ buffer, 0, size };
    bufferInfos.push_back({ bufferInfo });
    bindingMap[name].descriptorCount = 1;

    vk::WriteDescriptorSet write = MakeWrite(bindingMap[name]);
    write.setBufferInfo(bufferInfos.back());
    writes.push_back(write);
}

void DescriptorSet::Register(const std::string& name, vk::ImageView view, vk::Sampler sampler)
{
    vk::DescriptorImageInfo imageInfo;
    imageInfo.setImageView(view);
    imageInfo.setSampler(sampler);
    imageInfo.setImageLayout(vk::ImageLayout::eGeneral);
    imageInfos.push_back({ imageInfo });
    bindingMap[name].descriptorCount = 1;

    vk::WriteDescriptorSet write = MakeWrite(bindingMap[name]);
    write.setImageInfo(imageInfos.back());
    writes.push_back(write);
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
    imageInfos.push_back(infos);

    bindingMap[name].descriptorCount = images.size();

    vk::WriteDescriptorSet write = MakeWrite(bindingMap[name]);
    write.setImageInfo(imageInfos.back());
    writes.push_back(write);
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
    accelInfos.push_back(accelInfo);
    bindingMap[name].descriptorCount = 1;

    vk::WriteDescriptorSet write = MakeWrite(bindingMap[name]);
    write.setPNext(&accelInfos.back());
    writes.push_back(write);
}
