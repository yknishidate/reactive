#pragma once
#include <fstream>
#include <sstream>
#include <iostream>
#include <vulkan/vulkan.hpp>
#include "Buffer.hpp"

class Pipeline
{
public:
    void UpdateDescSet(const std::string& name, vk::ImageView view, vk::Sampler sampler);

protected:
    vk::UniqueDescriptorSetLayout descSetLayout;
    vk::UniquePipelineLayout pipelineLayout;
    vk::UniquePipeline pipeline;
    vk::UniqueDescriptorSet descSet;
    size_t pushSize;
    std::unordered_map<std::string, vk::DescriptorSetLayoutBinding> bindingMap;
};

class ComputePipeline : public Pipeline
{
public:
    void Init(const std::string& path, size_t pushSize = 0);
    void Run(vk::CommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, void* pushData = nullptr);

private:
    vk::UniqueShaderModule shaderModule;
};

class RayTracingPipeline : public Pipeline
{
public:
    void Init(const std::string& rgenPath, const std::string& missPath, const std::string& chitPath, size_t pushSize = 0);
    void Run(vk::CommandBuffer commandBuffer, uint32_t countX, uint32_t countY, void* pushData = nullptr);

private:
    std::vector<vk::UniqueShaderModule> shaderModules;
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups;

    vk::StridedDeviceAddressRegionKHR raygenRegion;
    vk::StridedDeviceAddressRegionKHR missRegion;
    vk::StridedDeviceAddressRegionKHR hitRegion;

    Buffer raygenSBT;
    Buffer missSBT;
    Buffer hitSBT;
};
