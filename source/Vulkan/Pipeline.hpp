#pragma once
#include <fstream>
#include <sstream>
#include <iostream>
#include <vulkan/vulkan.hpp>
#include "Buffer.hpp"

class Image;

class Pipeline
{
public:
    void Register(const std::string& name, vk::Buffer buffer, size_t size);
    void Register(const std::string& name, vk::ImageView view, vk::Sampler sampler);
    void Register(const std::string& name, const std::vector<Image>& images);
    void Register(const std::string& name, const Buffer& buffer);
    void Register(const std::string& name, const Image& image);
    void Register(const std::string& name, const vk::AccelerationStructureKHR& accel);

protected:
    vk::UniqueDescriptorSetLayout descSetLayout;
    vk::UniquePipelineLayout pipelineLayout;
    vk::UniquePipeline pipeline;
    vk::UniqueDescriptorSet descSet;
    size_t pushSize;
    std::unordered_map<std::string, vk::DescriptorSetLayoutBinding> bindingMap;

    std::vector<std::vector<vk::DescriptorImageInfo>> imageInfos;
    std::vector<std::vector<vk::DescriptorBufferInfo>> bufferInfos;
    std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> accelInfos;
    std::vector<vk::WriteDescriptorSet> writes;
};

class ComputePipeline : public Pipeline
{
public:
    void LoadShaders(const std::string& path);
    void Setup(size_t pushSize = 0);
    void Run(vk::CommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, void* pushData = nullptr);

private:
    vk::UniqueShaderModule shaderModule;
};

class RayTracingPipeline : public Pipeline
{
public:
    void LoadShaders(const std::string& rgenPath, const std::string& missPath, const std::string& chitPath);
    void LoadShaders(const std::string& rgenPath, const std::string& missPath, const std::string& chitPath, const std::string& ahitPath);
    void Setup(size_t pushSize = 0);
    void Run(vk::CommandBuffer commandBuffer, uint32_t countX, uint32_t countY, void* pushData = nullptr);

private:
    std::vector<vk::UniqueShaderModule> shaderModules;
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups;

    vk::StridedDeviceAddressRegionKHR raygenRegion;
    vk::StridedDeviceAddressRegionKHR missRegion;
    vk::StridedDeviceAddressRegionKHR hitRegion;

    DeviceBuffer raygenSBT;
    DeviceBuffer missSBT;
    DeviceBuffer hitSBT;

    int rgenCount = 0;
    int missCount = 0;
    int hitCount = 0;
};
