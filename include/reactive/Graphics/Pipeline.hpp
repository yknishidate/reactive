#pragma once
#include <variant>
#include <vulkan/vulkan.hpp>
#include "ArrayProxy.hpp"
#include "DescriptorSet.hpp"
#include "Scene/Mesh.hpp"

namespace rv {
class Image;

struct GraphicsPipelineCreateInfo {
    // Layout
    vk::DescriptorSetLayout descSetLayout = {};

    uint32_t pushSize = 0;

    // Shader
    ShaderHandle vertexShader;
    ShaderHandle fragmentShader;

    // Vertex
    uint32_t vertexStride = 0;
    ArrayProxy<VertexAttributeDescription> vertexAttributes = {};

    // Viewport
    ArrayProxy<vk::Format> colorFormats;
    vk::Format depthFormat;

    // Vertex input
    vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;

    // Raster
    std::variant<vk::PolygonMode, std::string> polygonMode = vk::PolygonMode::eFill;
    std::variant<vk::CullModeFlags, std::string> cullMode = vk::CullModeFlagBits::eNone;
    std::variant<vk::FrontFace, std::string> frontFace = vk::FrontFace::eCounterClockwise;
    std::variant<float, std::string> lineWidth = 1.0f;

    // Color blend
    bool alphaBlending = false;
};

struct ComputePipelineCreateInfo {
    vk::DescriptorSetLayout descSetLayout = {};
    uint32_t pushSize = 0;
    ShaderHandle computeShader;
};

struct MeshShaderPipelineCreateInfo {
    vk::DescriptorSetLayout descSetLayout = {};
    uint32_t pushSize = 0;
    ShaderHandle taskShader;
    ShaderHandle meshShader;
    ShaderHandle fragmentShader;

    // Viewport
    ArrayProxy<vk::Format> colorFormats;
    vk::Format depthFormat;

    // Raster
    std::variant<vk::PolygonMode, std::string> polygonMode = vk::PolygonMode::eFill;
    std::variant<vk::CullModeFlags, std::string> cullMode = vk::CullModeFlagBits::eNone;
    std::variant<vk::FrontFace, std::string> frontFace = vk::FrontFace::eCounterClockwise;
    std::variant<float, std::string> lineWidth = 1.0f;

    // Color blend
    bool alphaBlending = false;
};

struct RaygenGroup {
    ShaderHandle raygenShader;
};

struct MissGroup {
    ShaderHandle missShader;
};

struct HitGroup {
    ShaderHandle chitShader;
    ShaderHandle ahitShader;
};

struct CallableGroup {
    Shader callableShader;
};

struct RayTracingPipelineCreateInfo {
    RaygenGroup rgenGroup;
    ArrayProxy<MissGroup> missGroups;
    ArrayProxy<HitGroup> hitGroups;
    ArrayProxy<CallableGroup> callableGroups;

    vk::DescriptorSetLayout descSetLayout = {};
    uint32_t pushSize = 0;

    uint32_t maxRayRecursionDepth = 4;
};

class Pipeline {
public:
    Pipeline(const Context& _context) : context{&_context} {}

    auto getPipelineBindPoint() const -> vk::PipelineBindPoint { return bindPoint; }
    auto getPipelineLayout() const -> vk::PipelineLayout { return *pipelineLayout; }

protected:
    friend class CommandBuffer;

    const Context* context = nullptr;
    vk::UniquePipelineLayout pipelineLayout;
    vk::UniquePipeline pipeline;
    vk::ShaderStageFlags shaderStageFlags;
    vk::PipelineBindPoint bindPoint = {};
    uint32_t pushSize = 0;
};

class GraphicsPipeline : public Pipeline {
public:
    GraphicsPipeline(const Context& _context, const GraphicsPipelineCreateInfo& createInfo);
};

class MeshShaderPipeline : public Pipeline {
public:
    MeshShaderPipeline(const Context& _context, const MeshShaderPipelineCreateInfo& createInfo);
};

class ComputePipeline : public Pipeline {
public:
    ComputePipeline(const Context& _context, const ComputePipelineCreateInfo& createInfo);
};

class RayTracingPipeline : public Pipeline {
public:
    RayTracingPipeline() = default;
    RayTracingPipeline(const Context& _context, const RayTracingPipelineCreateInfo& createInfo);

private:
    friend class CommandBuffer;

    void createSBT();

    std::vector<vk::ShaderModule> shaderModules;
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups;

    vk::StridedDeviceAddressRegionKHR raygenRegion;
    vk::StridedDeviceAddressRegionKHR missRegion;
    vk::StridedDeviceAddressRegionKHR hitRegion;
    vk::StridedDeviceAddressRegionKHR callableRegion;

    BufferHandle sbtBuffer;

    uint32_t rgenCount = 0;
    uint32_t missCount = 0;
    uint32_t hitCount = 0;
};
}  // namespace rv
