#pragma once
#include <variant>
#include <vulkan/vulkan.hpp>
#include "ArrayProxy.hpp"
#include "Buffer.hpp"
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
    vk::Format colorFormat = vk::Format::eB8G8R8A8Unorm;
    vk::Format depthFormat = vk::Format::eD32Sfloat;

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
    ShaderHandle computeShader;

    vk::DescriptorSetLayout descSetLayout = {};
    uint32_t pushSize = 0;
};

struct MeshShaderPipelineCreateInfo {
    vk::DescriptorSetLayout descSetLayout = {};
    uint32_t pushSize = 0;
    ShaderHandle taskShader;
    ShaderHandle meshShader;
    ShaderHandle fragmentShader;

    // Viewport
    std::variant<vk::Viewport, std::string> viewport;
    std::variant<vk::Rect2D, std::string> scissor;

    // Raster
    vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
    std::variant<vk::CullModeFlags, std::string> cullMode = vk::CullModeFlagBits::eNone;
    std::variant<vk::FrontFace, std::string> frontFace = vk::FrontFace::eCounterClockwise;
    std::variant<float, std::string> lineWidth = 1.0f;

    // Color blend
    bool alphaBlending = false;
};

struct RayTracingPipelineCreateInfo {
    ArrayProxy<ShaderHandle> rgenShaders;
    ArrayProxy<ShaderHandle> missShaders;
    ArrayProxy<ShaderHandle> chitShaders;
    ArrayProxy<ShaderHandle> ahitShaders;  // not supported

    vk::DescriptorSetLayout descSetLayout = {};
    uint32_t pushSize = 0;

    uint32_t maxRayRecursionDepth = 4;
};

class Pipeline {
public:
    Pipeline(const Context* context) : context{context} {}

    auto getPipelineBindPoint() const -> vk::PipelineBindPoint { return bindPoint; }
    auto getPipelineLayout() const -> vk::PipelineLayout { return *pipelineLayout; }

protected:
    friend class CommandBuffer;

    const Context* context;
    vk::UniquePipelineLayout pipelineLayout;
    vk::UniquePipeline pipeline;
    vk::ShaderStageFlags shaderStageFlags;
    vk::PipelineBindPoint bindPoint;
    uint32_t pushSize = 0;
};

class GraphicsPipeline : public Pipeline {
public:
    GraphicsPipeline(const Context* context, GraphicsPipelineCreateInfo createInfo);
};

class MeshShaderPipeline : public Pipeline {
public:
    MeshShaderPipeline(const Context* context, MeshShaderPipelineCreateInfo createInfo);
};

class ComputePipeline : public Pipeline {
public:
    ComputePipeline(const Context* context, ComputePipelineCreateInfo createInfo);
};

class RayTracingPipeline : public Pipeline {
public:
    RayTracingPipeline() = default;
    RayTracingPipeline(const Context* context, RayTracingPipelineCreateInfo createInfo);

private:
    friend class CommandBuffer;

    std::vector<vk::ShaderModule> shaderModules;
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups;

    vk::StridedDeviceAddressRegionKHR raygenRegion;
    vk::StridedDeviceAddressRegionKHR missRegion;
    vk::StridedDeviceAddressRegionKHR hitRegion;

    BufferHandle raygenSBT;
    BufferHandle missSBT;
    BufferHandle hitSBT;

    uint32_t rgenCount = 0;
    uint32_t missCount = 0;
    uint32_t hitCount = 0;
};
}  // namespace rv
