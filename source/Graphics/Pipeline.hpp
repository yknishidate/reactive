#pragma once
#include <memory>
#include <vulkan/vulkan.hpp>
#include "ArrayProxy.hpp"
#include "Buffer.hpp"
#include "DescriptorSet.hpp"
#include "Scene/Mesh.hpp"

class Image;
class Scene;
// class RenderPass;

class Pipeline {
public:
    Pipeline() = default;
    Pipeline(const Context* context) : context{context} {}

    vk::PipelineBindPoint getPipelineBindPoint() const { return bindPoint; }
    vk::PipelineLayout getPipelineLayout() const { return *pipelineLayout; }

protected:
    friend class CommandBuffer;

    void bind(vk::CommandBuffer commandBuffer) { commandBuffer.bindPipeline(bindPoint, *pipeline); }

    void pushConstants(vk::CommandBuffer commandBuffer, const void* pushData) {
        commandBuffer.pushConstants(*pipelineLayout, shaderStageFlags, 0, pushSize, pushData);
    }

    const Context* context;
    vk::UniquePipelineLayout pipelineLayout;
    vk::UniquePipeline pipeline;
    vk::ShaderStageFlags shaderStageFlags;
    vk::PipelineBindPoint bindPoint;
    uint32_t pushSize = 0;
};

struct GraphicsPipelineCreateInfo {
    const Shader* vertexShader = nullptr;
    const Shader* fragmentShader = nullptr;

    vk::RenderPass renderPass = {};
    vk::DescriptorSetLayout descSetLayout = {};
    uint32_t pushSize = 0;

    // Rasterization state
    vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
    vk::CullModeFlags cullMode = vk::CullModeFlagBits::eNone;
    vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
    float lineWidth = 1.0f;

    // Viewport state
    uint32_t width = 0;
    uint32_t height = 0;

    // Vertex input state
    uint32_t vertexStride = 0;
    ArrayProxy<vk::VertexInputAttributeDescription> vertexAttributes = {};

    // Input assembly state
    vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
};

class GraphicsPipeline : public Pipeline {
public:
    GraphicsPipeline() = default;
    GraphicsPipeline(const Context* context, GraphicsPipelineCreateInfo createInfo);

private:
    enum class Type { Graphics, MeshShader };
    Type type = Type::Graphics;
};

// class ComputePipeline : public Pipeline {
// public:
//     ComputePipeline() = default;
//     ComputePipeline(const Context* context) : Pipeline{app} {}
//
//     void setup();
//
//     void setComputeShader(const Shader& compShader) {
//         assert(compShader.getStage() == vk::ShaderStageFlagBits::eCompute);
//         shaderModule = compShader.getModule();
//     }
//
// private:
//     friend class CommandBuffer;
//     void bind(vk::CommandBuffer commandBuffer) override;
//     void pushConstants(vk::CommandBuffer commandBuffer, const void* pushData) override;
//     void dispatch(vk::CommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY);
//
//     vk::ShaderModule shaderModule;
// };
//
// class RayTracingPipeline : public Pipeline {
// public:
//     RayTracingPipeline() = default;
//     RayTracingPipeline(const Context* context) : Pipeline{app} {}
//
//     void setup();
//
//     void setShaders(const Shader& rgenShader, const Shader& missShader, const Shader& chitShader)
//     {
//         assert(rgenShader.getStage() == vk::ShaderStageFlagBits::eRaygenKHR);
//         assert(missShader.getStage() == vk::ShaderStageFlagBits::eMissKHR);
//         assert(chitShader.getStage() == vk::ShaderStageFlagBits::eClosestHitKHR);
//
//         rgenCount = 1;
//         missCount = 1;
//         hitCount = 1;
//
//         shaderModules.push_back(rgenShader.getModule());
//         shaderModules.push_back(missShader.getModule());
//         shaderModules.push_back(chitShader.getModule());
//
//         shaderStages.push_back({{}, vk::ShaderStageFlagBits::eRaygenKHR, shaderModules[0],
//         "main"}); shaderStages.push_back({{}, vk::ShaderStageFlagBits::eMissKHR,
//         shaderModules[1], "main"}); shaderStages.push_back(
//             {{}, vk::ShaderStageFlagBits::eClosestHitKHR, shaderModules[2], "main"});
//
//         shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eGeneral, 0,
//         VK_SHADER_UNUSED_KHR,
//                                 VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR});
//
//         shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eGeneral, 1,
//         VK_SHADER_UNUSED_KHR,
//                                 VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR});
//
//         shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
//                                 VK_SHADER_UNUSED_KHR, 2, VK_SHADER_UNUSED_KHR,
//                                 VK_SHADER_UNUSED_KHR});
//     }
//
//     void setShaders(const Shader& rgenShader,
//                     const Shader& missShader,
//                     const Shader& chitShader,
//                     const Shader& ahitShader) {
//         assert(rgenShader.getStage() == vk::ShaderStageFlagBits::eRaygenKHR);
//         assert(missShader.getStage() == vk::ShaderStageFlagBits::eMissKHR);
//         assert(chitShader.getStage() == vk::ShaderStageFlagBits::eClosestHitKHR);
//         assert(ahitShader.getStage() == vk::ShaderStageFlagBits::eAnyHitKHR);
//
//         rgenCount = 1;
//         missCount = 1;
//         hitCount = 2;
//
//         shaderModules.push_back(rgenShader.getModule());
//         shaderModules.push_back(missShader.getModule());
//         shaderModules.push_back(chitShader.getModule());
//         shaderModules.push_back(ahitShader.getModule());
//
//         shaderStages.push_back({{}, vk::ShaderStageFlagBits::eRaygenKHR, shaderModules[0],
//         "main"}); shaderStages.push_back({{}, vk::ShaderStageFlagBits::eMissKHR,
//         shaderModules[1], "main"}); shaderStages.push_back(
//             {{}, vk::ShaderStageFlagBits::eClosestHitKHR, shaderModules[2], "main"});
//         shaderStages.push_back({{}, vk::ShaderStageFlagBits::eAnyHitKHR, shaderModules[3],
//         "main"});
//
//         shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eGeneral, 0,
//         VK_SHADER_UNUSED_KHR,
//                                 VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR});
//
//         shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eGeneral, 1,
//         VK_SHADER_UNUSED_KHR,
//                                 VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR});
//
//         shaderGroups.push_back({vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
//                                 VK_SHADER_UNUSED_KHR, 2, 3, VK_SHADER_UNUSED_KHR});
//     }
//
// private:
//     friend class CommandBuffer;
//     void bind(vk::CommandBuffer commandBuffer) override;
//     void pushConstants(vk::CommandBuffer commandBuffer, const void* pushData) override;
//     void traceRays(vk::CommandBuffer commandBuffer, uint32_t countX, uint32_t countY);
//
//     std::vector<vk::ShaderModule> shaderModules;
//     std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
//     std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups;
//
//     vk::StridedDeviceAddressRegionKHR raygenRegion;
//     vk::StridedDeviceAddressRegionKHR missRegion;
//     vk::StridedDeviceAddressRegionKHR hitRegion;
//
//     DeviceBuffer raygenSBT;
//     DeviceBuffer missSBT;
//     DeviceBuffer hitSBT;
//
//     int rgenCount = 0;
//     int missCount = 0;
//     int hitCount = 0;
// };
