#include "Vulkan.hpp"
#include "Pipeline.hpp"
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/ResourceLimits.h>
#include <spirv_glsl.hpp>
#include <spdlog/spdlog.h>

namespace
{
    std::string ReadFile(const std::string& path)
    {
        std::ifstream input_file{ path };
        if (!input_file.is_open()) {
            throw std::runtime_error("failed to open file: " + path);
        }
        return { (std::istreambuf_iterator<char>{input_file}), std::istreambuf_iterator<char>{} };
    }

    std::vector<unsigned int> CompileToSPV(const std::string& glslShader, EShLanguage stage)
    {
        glslang::InitializeProcess();

        const char* shaderStrings[1];
        shaderStrings[0] = glslShader.data();

        glslang::TShader shader(stage);
        shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_4);
        shader.setStrings(shaderStrings, 1);

        EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

        if (!shader.parse(&glslang::DefaultTBuiltInResource, 100, false, messages)) {
            throw std::runtime_error(glslShader + "\n" + shader.getInfoLog());
        }

        glslang::TProgram program;
        program.addShader(&shader);

        if (!program.link(messages)) {
            throw std::runtime_error(glslShader + "\n" + shader.getInfoLog());
        }

        std::vector<unsigned int> spvShader;
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

    void AddBindingMap(const std::vector<unsigned int>& spvShader,
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

    vk::UniqueShaderModule CreateShaderModule(const std::vector<unsigned int>& code)
    {
        vk::ShaderModuleCreateInfo createInfo;
        createInfo.setCode(code);
        return Vulkan::Device.createShaderModuleUnique(createInfo);
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
        return Vulkan::Device.createDescriptorSetLayoutUnique(createInfo);
    }

    vk::UniquePipelineLayout CreatePipelineLayout(vk::DescriptorSetLayout descSetLayout,
                                                  size_t pushSize, vk::ShaderStageFlags shaderStage)
    {
        vk::PipelineLayoutCreateInfo createInfo;
        createInfo.setSetLayouts(descSetLayout);
        vk::PushConstantRange pushRange;
        if (pushSize) {
            pushRange.setOffset(0);
            pushRange.setSize(pushSize);
            pushRange.setStageFlags(shaderStage);
            createInfo.setPushConstantRanges(pushRange);
        }
        return Vulkan::Device.createPipelineLayoutUnique(createInfo);
    }

    vk::UniquePipeline CreateComputePipeline(vk::ShaderModule shaderModule, vk::ShaderStageFlagBits shaderStage,
                                             vk::PipelineLayout pipelineLayout)
    {
        vk::PipelineShaderStageCreateInfo stage;
        stage.setStage(shaderStage);
        stage.setModule(shaderModule);
        stage.setPName("main");

        vk::ComputePipelineCreateInfo createInfo;
        createInfo.setStage(stage);
        createInfo.setLayout(pipelineLayout);
        auto res = Vulkan::Device.createComputePipelinesUnique({}, createInfo);
        if (res.result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create ray tracing pipeline.");
        }
        return std::move(res.value.front());
    }

    vk::UniquePipeline CreateRayTracingPipeline(const std::vector<vk::PipelineShaderStageCreateInfo>& shaderStages,
                                                const std::vector<vk::RayTracingShaderGroupCreateInfoKHR>& shaderGroups,
                                                vk::PipelineLayout pipelineLayout)
    {
        vk::RayTracingPipelineCreateInfoKHR createInfo;
        createInfo.setStages(shaderStages);
        createInfo.setGroups(shaderGroups);
        createInfo.setMaxPipelineRayRecursionDepth(4);
        createInfo.setLayout(pipelineLayout);
        auto res = Vulkan::Device.createRayTracingPipelineKHRUnique(nullptr, nullptr, createInfo);
        if (res.result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create ray tracing pipeline.");
        }
        return std::move(res.value);
        return {};
    }

    vk::UniqueDescriptorSet AllocateDescSet(vk::DescriptorSetLayout descSetLayout)
    {
        vk::DescriptorSetAllocateInfo allocateInfo;
        allocateInfo.setDescriptorPool(Vulkan::DescriptorPool);
        allocateInfo.setSetLayouts(descSetLayout);
        std::vector descSets = Vulkan::Device.allocateDescriptorSetsUnique(allocateInfo);
        return std::move(descSets.front());
    }

    vk::StridedDeviceAddressRegionKHR CreateAddressRegion(
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties,
        vk::DeviceAddress deviceAddress)
    {
        vk::StridedDeviceAddressRegionKHR region{};
        region.setDeviceAddress(deviceAddress);
        region.setStride(rtProperties.shaderGroupHandleAlignment);
        region.setSize(rtProperties.shaderGroupHandleAlignment);
        return region;
    }

    vk::WriteDescriptorSet MakeWrite(vk::DescriptorSetLayoutBinding binding)
    {
        vk::WriteDescriptorSet write;
        write.setDescriptorType(binding.descriptorType);
        write.setDescriptorCount(binding.descriptorCount);
        write.setDstBinding(binding.binding);
        return write;
    }
} // namespace

void Pipeline::UpdateDescSet(const std::string& name, vk::Buffer buffer, size_t size)
{
    vk::DescriptorBufferInfo bufferInfo{ buffer, 0, size };

    vk::WriteDescriptorSet write = MakeWrite(bindingMap[name]);
    write.setDstSet(*descSet);
    write.setBufferInfo(bufferInfo);
    Vulkan::Device.updateDescriptorSets(write, nullptr);
}

void Pipeline::UpdateDescSet(const std::string& name, vk::ImageView view, vk::Sampler sampler)
{
    vk::DescriptorImageInfo descImageInfo;
    descImageInfo.setImageView(view);
    descImageInfo.setSampler(sampler);
    descImageInfo.setImageLayout(vk::ImageLayout::eGeneral);

    vk::WriteDescriptorSet write = MakeWrite(bindingMap[name]);
    write.setDstSet(*descSet);
    write.setImageInfo(descImageInfo);
    Vulkan::Device.updateDescriptorSets(write, nullptr);
}

void Pipeline::UpdateDescSet(const std::string& name, vk::AccelerationStructureKHR accel)
{
    vk::WriteDescriptorSetAccelerationStructureKHR accelInfo{ accel };

    vk::WriteDescriptorSet write = MakeWrite(bindingMap[name]);
    write.setDstSet(*descSet);
    write.setPNext(&accelInfo);
    Vulkan::Device.updateDescriptorSets(write, nullptr);
}

void ComputePipeline::Init(const std::string& path, size_t pushSize)
{
    spdlog::info("ComputePipeline::Init()");
    this->pushSize = pushSize;

    std::vector spirvCode = CompileToSPV(ReadFile(path), EShLangCompute);
    AddBindingMap(spirvCode, bindingMap, vk::ShaderStageFlagBits::eCompute);

    shaderModule = CreateShaderModule(spirvCode);
    descSetLayout = CreateDescSetLayout(GetBindings(bindingMap));
    pipelineLayout = CreatePipelineLayout(*descSetLayout, pushSize, vk::ShaderStageFlagBits::eCompute);
    pipeline = CreateComputePipeline(*shaderModule, vk::ShaderStageFlagBits::eCompute, *pipelineLayout);
    descSet = AllocateDescSet(*descSetLayout);
}

void ComputePipeline::Run(vk::CommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, void* pushData)
{
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipelineLayout, 0, *descSet, nullptr);
    if (pushData) {
        commandBuffer.pushConstants(*pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, pushSize, pushData);
    }
    commandBuffer.dispatch(groupCountX, groupCountY, 1);
}

void RayTracingPipeline::Init(const std::string& rgenPath, const std::string& missPath, const std::string& chitPath, size_t pushSize)
{
    spdlog::info("RayTracingPipeline::Init()");
    this->pushSize = pushSize;
    const uint32_t rgenIndex = 0;
    const uint32_t missIndex = 1;
    const uint32_t chitIndex = 2;

    // Compile shaders
    std::vector rgenShader = CompileToSPV(ReadFile(rgenPath), EShLangRayGen);
    std::vector missShader = CompileToSPV(ReadFile(missPath), EShLangMiss);
    std::vector chitShader = CompileToSPV(ReadFile(chitPath), EShLangClosestHit);

    // Get bindings
    AddBindingMap(rgenShader, bindingMap, vk::ShaderStageFlagBits::eRaygenKHR);
    AddBindingMap(missShader, bindingMap, vk::ShaderStageFlagBits::eMissKHR);
    AddBindingMap(chitShader, bindingMap, vk::ShaderStageFlagBits::eClosestHitKHR);

    std::vector bindings = GetBindings(bindingMap);

    shaderModules.push_back(CreateShaderModule(rgenShader));
    shaderStages.push_back({ {}, vk::ShaderStageFlagBits::eRaygenKHR, *shaderModules.back(), "main" });
    shaderGroups.push_back({ vk::RayTracingShaderGroupTypeKHR::eGeneral,
                           rgenIndex, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR });

    shaderModules.push_back(CreateShaderModule(missShader));
    shaderStages.push_back({ {}, vk::ShaderStageFlagBits::eMissKHR, *shaderModules.back(), "main" });
    shaderGroups.push_back({ vk::RayTracingShaderGroupTypeKHR::eGeneral,
                           missIndex, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR });

    shaderModules.push_back(CreateShaderModule(chitShader));
    shaderStages.push_back({ {}, vk::ShaderStageFlagBits::eClosestHitKHR, *shaderModules.back(), "main" });
    shaderGroups.push_back({ vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
                           VK_SHADER_UNUSED_KHR, chitIndex, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR });

    descSetLayout = CreateDescSetLayout(bindings);
    pipelineLayout = CreatePipelineLayout(*descSetLayout, pushSize, vk::ShaderStageFlagBits::eRaygenKHR);
    pipeline = CreateRayTracingPipeline(shaderStages, shaderGroups, *pipelineLayout);

    // Get Ray Tracing Properties
    using vkRTP = vk::PhysicalDeviceRayTracingPipelinePropertiesKHR;
    auto rtProperties = Vulkan::PhysicalDevice.getProperties2<vk::PhysicalDeviceProperties2, vkRTP>().get<vkRTP>();

    // Calculate SBT size
    uint32_t handleSize = rtProperties.shaderGroupHandleSize;
    size_t handleSizeAligned = rtProperties.shaderGroupHandleAlignment;
    size_t groupCount = shaderGroups.size();
    size_t sbtSize = groupCount * handleSizeAligned;

    // Get shader group handles
    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    vk::Result groupRes = Vulkan::Device.getRayTracingShaderGroupHandlesKHR(*pipeline, 0, groupCount, sbtSize,
                                                                            shaderHandleStorage.data());
    if (groupRes != vk::Result::eSuccess) {
        throw std::runtime_error("failed to get ray tracing shader group handles.");
    }

    // Create shader binding table
    vk::BufferUsageFlags usage =
        vk::BufferUsageFlagBits::eShaderBindingTableKHR |
        vk::BufferUsageFlagBits::eTransferSrc |
        vk::BufferUsageFlagBits::eShaderDeviceAddress;

    raygenSBT.InitOnHost(handleSize, usage);
    missSBT.InitOnHost(handleSize, usage);
    hitSBT.InitOnHost(handleSize, usage);

    raygenRegion = CreateAddressRegion(rtProperties, raygenSBT.GetAddress());
    missRegion = CreateAddressRegion(rtProperties, missSBT.GetAddress());
    hitRegion = CreateAddressRegion(rtProperties, hitSBT.GetAddress());

    raygenSBT.Copy(shaderHandleStorage.data() + 0 * handleSizeAligned);
    missSBT.Copy(shaderHandleStorage.data() + 1 * handleSizeAligned);
    hitSBT.Copy(shaderHandleStorage.data() + 2 * handleSizeAligned);

    descSet = AllocateDescSet(*descSetLayout);
}

void RayTracingPipeline::Run(vk::CommandBuffer commandBuffer, uint32_t countX, uint32_t countY, void* pushData)
{
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, *pipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, *pipelineLayout, 0, *descSet, nullptr);
    if (pushData) {
        commandBuffer.pushConstants(*pipelineLayout, vk::ShaderStageFlagBits::eRaygenKHR, 0, pushSize, pushData);
    }
    commandBuffer.traceRaysKHR(raygenRegion, missRegion, hitRegion, {}, countX, countY, 1);
}
