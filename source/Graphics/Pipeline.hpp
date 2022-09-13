#pragma once
#include <memory>
#include <vulkan/vulkan.hpp>
#include "Buffer.hpp"
#include "DescriptorSet.hpp"

class Image;
class Scene;

class Pipeline
{
public:
    Pipeline()
    {
        descSet = std::make_shared<DescriptorSet>();
    }

    Pipeline(std::shared_ptr<DescriptorSet> descSet)
    {
        this->descSet = descSet;
    }

    void Register(const std::string& name, const std::vector<Image>& images);
    void Register(const std::string& name, const Buffer& buffer);
    void Register(const std::string& name, const Image& image);
    void Register(const std::string& name, const TopAccel& accel);

    DescriptorSet& GetDescSet() { return *descSet; }

protected:
    vk::UniquePipelineLayout pipelineLayout;
    vk::UniquePipeline pipeline;
    size_t pushSize{};
    std::shared_ptr<DescriptorSet> descSet;
    bool registered = false;
};

class GraphicsPipeline : public Pipeline
{
public:
    GraphicsPipeline() : Pipeline() {}
    GraphicsPipeline(std::shared_ptr<DescriptorSet> descSet) : Pipeline(descSet) {}

    virtual void LoadShaders(const std::string& vertPath, const std::string& fragPath);
    virtual void Setup(size_t pushSize = 0);
    virtual void Begin(vk::CommandBuffer commandBuffer, void* pushData = nullptr);
    virtual void End(vk::CommandBuffer commandBuffer);

private:
    vk::UniqueShaderModule vertModule;
    vk::UniqueShaderModule fragModule;
};

class ComputePipeline : public Pipeline
{
public:
    ComputePipeline() : Pipeline() {}
    ComputePipeline(std::shared_ptr<DescriptorSet> descSet) : Pipeline(descSet) {}

    virtual void LoadShaders(const std::string& path);
    virtual void Setup(size_t pushSize = 0);
    virtual void Run(vk::CommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, void* pushData = nullptr);

private:
    vk::UniqueShaderModule shaderModule;
};

class RayTracingPipeline : public Pipeline
{
public:
    RayTracingPipeline() : Pipeline() {}
    RayTracingPipeline(std::shared_ptr<DescriptorSet> descSet) : Pipeline(descSet) {}

    virtual void LoadShaders(const std::string& rgenPath, const std::string& missPath, const std::string& chitPath);
    virtual void LoadShaders(const std::string& rgenPath, const std::string& missPath, const std::string& chitPath, const std::string& ahitPath);
    virtual void Setup(size_t pushSize = 0);
    virtual void Run(vk::CommandBuffer commandBuffer, uint32_t countX, uint32_t countY, void* pushData = nullptr);

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
