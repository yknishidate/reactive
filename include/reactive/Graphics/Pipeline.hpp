#pragma once
#include <variant>
#include <vulkan/vulkan.hpp>
#include "ArrayProxy.hpp"
#include "DescriptorSet.hpp"
#include "reactive/Scene/Mesh.hpp"

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
    Pipeline(const Context& context) : m_context{&context} {}

    auto getPipelineBindPoint() const -> vk::PipelineBindPoint { return m_bindPoint; }
    auto getPipelineLayout() const -> vk::PipelineLayout { return *m_pipelineLayout; }

protected:
    friend class CommandBuffer;

    const Context* m_context = nullptr;
    vk::UniquePipelineLayout m_pipelineLayout;
    vk::UniquePipeline m_pipeline;
    vk::ShaderStageFlags m_shaderStageFlags;
    vk::PipelineBindPoint m_bindPoint = {};
    uint32_t m_pushSize = 0;
};

class GraphicsPipeline : public Pipeline {
public:
    GraphicsPipeline(const Context& context, const GraphicsPipelineCreateInfo& createInfo);
};

class MeshShaderPipeline : public Pipeline {
public:
    MeshShaderPipeline(const Context& context, const MeshShaderPipelineCreateInfo& createInfo);
};

class ComputePipeline : public Pipeline {
public:
    ComputePipeline(const Context& context, const ComputePipelineCreateInfo& createInfo);
};

class RayTracingPipeline : public Pipeline {
public:
    RayTracingPipeline() = default;
    RayTracingPipeline(const Context& context, const RayTracingPipelineCreateInfo& createInfo);

private:
    friend class CommandBuffer;

    void createSBT();

    std::vector<vk::ShaderModule> m_shaderModules;
    std::vector<vk::PipelineShaderStageCreateInfo> m_shaderStages;
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> m_shaderGroups;

    vk::StridedDeviceAddressRegionKHR m_raygenRegion;
    vk::StridedDeviceAddressRegionKHR m_missRegion;
    vk::StridedDeviceAddressRegionKHR m_hitRegion;
    vk::StridedDeviceAddressRegionKHR m_callableRegion;

    BufferHandle m_sbtBuffer;

    uint32_t m_rgenCount = 0;
    uint32_t m_missCount = 0;
    uint32_t m_hitCount = 0;
};
}  // namespace rv
