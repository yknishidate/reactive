#pragma once
#include <memory>
#include <vulkan/vulkan.hpp>
#include "Buffer.hpp"
#include "DescriptorSet.hpp"

class Image;
class Scene;
// class RenderPass;

class Pipeline {
public:
    Pipeline() = default;
    Pipeline(const App* app) : m_app{app} {}

    void setDescriptorSet(DescriptorSet& set) { descSet = &set; }
    void setPushSize(size_t size) { pushSize = size; }

protected:
    friend class CommandBuffer;
    virtual void bind(vk::CommandBuffer commandBuffer) = 0;
    virtual void pushConstants(vk::CommandBuffer commandBuffer, const void* pushData) = 0;

    const App* m_app;
    vk::UniquePipelineLayout pipelineLayout;
    vk::UniquePipeline pipeline;
    size_t pushSize = 0;
    DescriptorSet* descSet;
};

class GraphicsPipeline : public Pipeline {
public:
    GraphicsPipeline() = default;
    GraphicsPipeline(const App* app) : Pipeline{app} {}

    // void setup(RenderPass& renderPass);
    void setup(vk::RenderPass renderPass);
    void addShader(const Shader& shader) { shaders.push_back(&shader); }

    void setTopology(vk::PrimitiveTopology topology) { this->topology = topology; }
    void setPolygonMode(vk::PolygonMode polygonMode) { this->polygonMode = polygonMode; }

private:
    friend class CommandBuffer;
    void bind(vk::CommandBuffer commandBuffer) override;
    void pushConstants(vk::CommandBuffer commandBuffer, const void* pushData) override;

    std::vector<const Shader*> shaders;
    vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
    vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
};

class ComputePipeline : public Pipeline {
public:
    ComputePipeline() = default;
    ComputePipeline(const App* app) : Pipeline{app} {}

    void setup();

    void setComputeShader(const Shader& compShader) {
        assert(compShader.getStage() == vk::ShaderStageFlagBits::eCompute);
        shaderModule = compShader.getModule();
    }

private:
    friend class CommandBuffer;
    void bind(vk::CommandBuffer commandBuffer) override;
    void pushConstants(vk::CommandBuffer commandBuffer, const void* pushData) override;
    void dispatch(vk::CommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY);

    vk::ShaderModule shaderModule;
};

class RayTracingPipeline : public Pipeline {
public:
    RayTracingPipeline() = default;
    RayTracingPipeline(const App* app) : Pipeline{app} {}

    void setup();

    void setShaders(const Shader& rgenShader, const Shader& missShader, const Shader& chitShader) {
        assert(rgenShader.getStage() == vk::ShaderStageFlagBits::eRaygenKHR);
        assert(missShader.getStage() == vk::ShaderStageFlagBits::eMissKHR);
        assert(chitShader.getStage() == vk::ShaderStageFlagBits::eClosestHitKHR);

        rgenCount = 1;
        missCount = 1;
        hitCount = 1;

        shaderModules.push_back(rgenShader.getModule());
        shaderModules.push_back(missShader.getModule());
        shaderModules.push_back(chitShader.getModule());

        shaderStages.push_back({{}, vk::ShaderStageFlagBits::eRaygenKHR, shaderModules[0], "main"});
        shaderStages.push_back({{}, vk::ShaderStageFlagBits::eMissKHR, shaderModules[1], "main"});
        shaderStages.push_back(
            {{}, vk::ShaderStageFlagBits::eClosestHitKHR, shaderModules[2], "main"});

        shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eGeneral, 0, VK_SHADER_UNUSED_KHR,
                                VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR});

        shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eGeneral, 1, VK_SHADER_UNUSED_KHR,
                                VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR});

        shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
                                VK_SHADER_UNUSED_KHR, 2, VK_SHADER_UNUSED_KHR,
                                VK_SHADER_UNUSED_KHR});
    }

    void setShaders(const Shader& rgenShader,
                    const Shader& missShader,
                    const Shader& chitShader,
                    const Shader& ahitShader) {
        assert(rgenShader.getStage() == vk::ShaderStageFlagBits::eRaygenKHR);
        assert(missShader.getStage() == vk::ShaderStageFlagBits::eMissKHR);
        assert(chitShader.getStage() == vk::ShaderStageFlagBits::eClosestHitKHR);
        assert(ahitShader.getStage() == vk::ShaderStageFlagBits::eAnyHitKHR);

        rgenCount = 1;
        missCount = 1;
        hitCount = 2;

        shaderModules.push_back(rgenShader.getModule());
        shaderModules.push_back(missShader.getModule());
        shaderModules.push_back(chitShader.getModule());
        shaderModules.push_back(ahitShader.getModule());

        shaderStages.push_back({{}, vk::ShaderStageFlagBits::eRaygenKHR, shaderModules[0], "main"});
        shaderStages.push_back({{}, vk::ShaderStageFlagBits::eMissKHR, shaderModules[1], "main"});
        shaderStages.push_back(
            {{}, vk::ShaderStageFlagBits::eClosestHitKHR, shaderModules[2], "main"});
        shaderStages.push_back({{}, vk::ShaderStageFlagBits::eAnyHitKHR, shaderModules[3], "main"});

        shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eGeneral, 0, VK_SHADER_UNUSED_KHR,
                                VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR});

        shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eGeneral, 1, VK_SHADER_UNUSED_KHR,
                                VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR});

        shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
                                VK_SHADER_UNUSED_KHR, 2, 3, VK_SHADER_UNUSED_KHR});
    }

private:
    friend class CommandBuffer;
    void bind(vk::CommandBuffer commandBuffer) override;
    void pushConstants(vk::CommandBuffer commandBuffer, const void* pushData) override;
    void traceRays(vk::CommandBuffer commandBuffer, uint32_t countX, uint32_t countY);

    std::vector<vk::ShaderModule> shaderModules;
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
