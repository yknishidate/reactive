#include <filesystem>
#include <regex>
#include "Context.hpp"
#include "Pipeline.hpp"
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/ResourceLimits.h>
#include <spirv_glsl.hpp>
#include <spdlog/spdlog.h>
#include "Image.hpp"
#include "Compiler.hpp"

namespace
{
    vk::UniqueShaderModule CreateShaderModule(const std::vector<uint32_t>& code)
    {
        vk::ShaderModuleCreateInfo createInfo;
        createInfo.setCode(code);
        return Context::GetDevice().createShaderModuleUnique(createInfo);
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
        return Context::GetDevice().createPipelineLayoutUnique(createInfo);
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
        auto res = Context::GetDevice().createComputePipelinesUnique({}, createInfo);
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
        auto res = Context::GetDevice().createRayTracingPipelineKHRUnique(nullptr, nullptr, createInfo);
        if (res.result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create ray tracing pipeline.");
        }
        return std::move(res.value);
        return {};
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

    void UpdateDescSet(vk::DescriptorSet descSet, std::vector<vk::WriteDescriptorSet>& writes)
    {
        for (auto&& write : writes) {
            write.setDstSet(descSet);
        }
        Context::GetDevice().updateDescriptorSets(writes, nullptr);
    }
} // namespace

void Pipeline::Register(const std::string& name, vk::Buffer buffer, size_t size)
{
    descSet.Register(name, buffer, size);
}

void Pipeline::Register(const std::string& name, vk::ImageView view, vk::Sampler sampler)
{
    descSet.Register(name, view, sampler);
}

void Pipeline::Register(const std::string& name, const std::vector<Image>& images)
{
    descSet.Register(name, images);
}

void Pipeline::Register(const std::string& name, const Buffer& buffer)
{
    Register(name, buffer.GetBuffer(), buffer.GetSize());
}

void Pipeline::Register(const std::string& name, const Image& image)
{
    Register(name, image.GetView(), image.GetSampler());
}

void Pipeline::Register(const std::string& name, const vk::AccelerationStructureKHR& accel)
{
    descSet.Register(name, accel);
}

void ComputePipeline::LoadShaders(const std::string& path)
{
    spdlog::info("ComputePipeline::LoadShaders()");

    std::vector spirvCode = Compiler::CompileToSPV(path);
    descSet.AddBindingMap(spirvCode, vk::ShaderStageFlagBits::eCompute);
    shaderModule = CreateShaderModule(spirvCode);
}

void ComputePipeline::Setup(size_t pushSize)
{
    this->pushSize = pushSize;
    descSet.SetupLayout();
    pipelineLayout = CreatePipelineLayout(descSet.GetLayout(), pushSize, vk::ShaderStageFlagBits::eCompute);
    pipeline = CreateComputePipeline(*shaderModule, vk::ShaderStageFlagBits::eCompute, *pipelineLayout);
    descSet.Allocate();
}

void ComputePipeline::Run(vk::CommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, void* pushData)
{
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipelineLayout, 0, descSet.Get(), nullptr);
    if (pushData) {
        commandBuffer.pushConstants(*pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, pushSize, pushData);
    }
    commandBuffer.dispatch(groupCountX, groupCountY, 1);
}

void RayTracingPipeline::LoadShaders(const std::string& rgenPath, const std::string& missPath, const std::string& chitPath)
{
    spdlog::info("RayTracingPipeline::LoadShaders()");
    this->pushSize = pushSize;
    rgenCount = 1;
    missCount = 1;
    hitCount = 1;

    const uint32_t rgenIndex = 0;
    const uint32_t missIndex = 1;
    const uint32_t chitIndex = 2;

    // Compile shaders
    std::vector rgenShader = Compiler::CompileToSPV(rgenPath);
    std::vector missShader = Compiler::CompileToSPV(missPath);
    std::vector chitShader = Compiler::CompileToSPV(chitPath);

    // Get bindings
    descSet.AddBindingMap(rgenShader, vk::ShaderStageFlagBits::eRaygenKHR);
    descSet.AddBindingMap(missShader, vk::ShaderStageFlagBits::eMissKHR);
    descSet.AddBindingMap(chitShader, vk::ShaderStageFlagBits::eClosestHitKHR);

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

}

void RayTracingPipeline::LoadShaders(const std::string& rgenPath, const std::string& missPath, const std::string& chitPath, const std::string& ahitPath)
{
    spdlog::info("RayTracingPipeline::LoadShaders()");
    this->pushSize = pushSize;
    rgenCount = 1;
    missCount = 1;
    hitCount = 2;

    const uint32_t rgenIndex = 0;
    const uint32_t missIndex = 1;
    const uint32_t chitIndex = 2;
    const uint32_t ahitIndex = 3;

    // Compile shaders
    std::vector rgenShader = Compiler::CompileToSPV(rgenPath);
    std::vector missShader = Compiler::CompileToSPV(missPath);
    std::vector chitShader = Compiler::CompileToSPV(chitPath);
    std::vector ahitShader = Compiler::CompileToSPV(ahitPath);

    // Get bindings
    descSet.AddBindingMap(rgenShader, vk::ShaderStageFlagBits::eRaygenKHR);
    descSet.AddBindingMap(missShader, vk::ShaderStageFlagBits::eMissKHR);
    descSet.AddBindingMap(chitShader, vk::ShaderStageFlagBits::eClosestHitKHR);
    descSet.AddBindingMap(ahitShader, vk::ShaderStageFlagBits::eAnyHitKHR);


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
    shaderModules.push_back(CreateShaderModule(ahitShader));
    shaderStages.push_back({ {}, vk::ShaderStageFlagBits::eAnyHitKHR, *shaderModules.back(), "main" });
    shaderGroups.push_back({ vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
                           VK_SHADER_UNUSED_KHR, chitIndex, ahitIndex, VK_SHADER_UNUSED_KHR });
}

void RayTracingPipeline::Setup(size_t pushSize)
{
    this->pushSize = pushSize;

    descSet.SetupLayout();
    pipelineLayout = CreatePipelineLayout(descSet.GetLayout(), pushSize,
                                          vk::ShaderStageFlagBits::eRaygenKHR |
                                          vk::ShaderStageFlagBits::eMissKHR |
                                          vk::ShaderStageFlagBits::eClosestHitKHR |
                                          vk::ShaderStageFlagBits::eAnyHitKHR);
    pipeline = CreateRayTracingPipeline(shaderStages, shaderGroups, *pipelineLayout);

    // Get Ray Tracing Properties
    using vkRTP = vk::PhysicalDeviceRayTracingPipelinePropertiesKHR;
    auto rtProperties = Context::GetPhysicalDevice().getProperties2<vk::PhysicalDeviceProperties2, vkRTP>().get<vkRTP>();

    // Calculate SBT size
    uint32_t handleSize = rtProperties.shaderGroupHandleSize;
    size_t handleSizeAligned = rtProperties.shaderGroupHandleAlignment;
    size_t groupCount = shaderGroups.size();
    size_t sbtSize = groupCount * handleSizeAligned;

    // Get shader group handles
    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    vk::Result groupRes = Context::GetDevice().getRayTracingShaderGroupHandlesKHR(*pipeline, 0, groupCount, sbtSize,
                                                                                  shaderHandleStorage.data());
    if (groupRes != vk::Result::eSuccess) {
        throw std::runtime_error("failed to get ray tracing shader group handles.");
    }

    // Create shader binding table
    vk::BufferUsageFlags usage =
        vk::BufferUsageFlagBits::eShaderBindingTableKHR |
        vk::BufferUsageFlagBits::eTransferDst |
        vk::BufferUsageFlagBits::eShaderDeviceAddress;

    raygenSBT = DeviceBuffer{ usage, handleSize * rgenCount };
    missSBT = DeviceBuffer{ usage, handleSize * missCount };
    hitSBT = DeviceBuffer{ usage, handleSize * hitCount };

    raygenRegion = CreateAddressRegion(rtProperties, raygenSBT.GetAddress());
    missRegion = CreateAddressRegion(rtProperties, missSBT.GetAddress());
    hitRegion = CreateAddressRegion(rtProperties, hitSBT.GetAddress());

    raygenSBT.Copy(shaderHandleStorage.data() + 0 * handleSizeAligned);
    missSBT.Copy(shaderHandleStorage.data() + 1 * handleSizeAligned);
    hitSBT.Copy(shaderHandleStorage.data() + 2 * handleSizeAligned);

    descSet.Allocate();
}

void RayTracingPipeline::Run(vk::CommandBuffer commandBuffer, uint32_t countX, uint32_t countY, void* pushData)
{
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, *pipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, *pipelineLayout, 0, descSet.Get(), nullptr);
    if (pushData) {
        commandBuffer.pushConstants(*pipelineLayout,
                                    vk::ShaderStageFlagBits::eRaygenKHR |
                                    vk::ShaderStageFlagBits::eMissKHR |
                                    vk::ShaderStageFlagBits::eClosestHitKHR |
                                    vk::ShaderStageFlagBits::eAnyHitKHR,
                                    0, pushSize, pushData);
    }
    commandBuffer.traceRaysKHR(raygenRegion, missRegion, hitRegion, {}, countX, countY, 1);
}
