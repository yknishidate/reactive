#pragma once
#include <fstream>
#include <sstream>
#include <iostream>
#include <vulkan/vulkan.hpp>
#include "Buffer.hpp"
#include "DescriptorSet.hpp"

class Image;
class Scene;

class Pipeline
{
public:
    void Register(const std::string& name, const std::vector<Image>& images);
    void Register(const std::string& name, const Buffer& buffer);
    void Register(const std::string& name, const Image& image);
    void Register(const std::string& name, const TopAccel& accel);

protected:
    vk::UniquePipelineLayout pipelineLayout;
    vk::UniquePipeline pipeline;
    size_t pushSize;
    DescriptorSet descSet;
};

class ComputePipeline : public Pipeline
{
public:
    virtual void LoadShaders(const std::string& path);
    virtual void Setup(size_t pushSize = 0);
    virtual void Run(vk::CommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, void* pushData = nullptr);

private:
    vk::UniqueShaderModule shaderModule;
};

class RayTracingPipeline : public Pipeline
{
public:
    virtual void LoadShaders(const std::string& rgenPath, const std::string& missPath, const std::string& chitPath);
    virtual void LoadShaders(const std::string& rgenPath, const std::string& missPath, const std::string& chitPath, const std::string& ahitPath);
    virtual void Setup(size_t pushSize = 0);
    virtual void Run(vk::CommandBuffer commandBuffer, uint32_t countX, uint32_t countY, void* pushData = nullptr);

    void RegisterScene(const Scene& scene);

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

struct GBuffers
{
    Image position;
    Image normal;
    Image diffuse;
    Image emission;
    Image instanceIndex;
};

class GBufferPipeline : public RayTracingPipeline
{
public:
    void LoadShaders();

    void Setup(size_t pushSize = 0);

    const GBuffers& GetGBuffers() const { return gbuffers; }

private:
    GBuffers gbuffers;
};

class UniformLightPipeline : public RayTracingPipeline
{
public:
    void LoadShaders();

    void RegisterGBuffers(const GBuffers& gbuffers)
    {
        Register("positionImage", gbuffers.position);
        Register("normalImage", gbuffers.normal);
        Register("diffuseImage", gbuffers.diffuse);
        Register("emissionImage", gbuffers.emission);
        Register("instanceIndexImage", gbuffers.instanceIndex);
    }

    void Setup(size_t pushSize = 0);

    const Image& GetOutputImage() const { return outputImage; }

private:
    Image outputImage;
};


class WRSPipeline : public RayTracingPipeline
{
public:
    void LoadShaders();

    void RegisterGBuffers(const GBuffers& gbuffers)
    {
        Register("positionImage", gbuffers.position);
        Register("normalImage", gbuffers.normal);
        Register("diffuseImage", gbuffers.diffuse);
        Register("emissionImage", gbuffers.emission);
        Register("instanceIndexImage", gbuffers.instanceIndex);
    }

    void Setup(size_t pushSize = 0);

    const Image& GetOutputImage() const { return outputImage; }

private:
    Image outputImage;
};
