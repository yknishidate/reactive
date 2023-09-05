#pragma once

#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include "Graphics/Buffer.hpp"

namespace rv {
struct VertexAttributeDescription {
    uint32_t offset;
    vk::Format format;
};

struct Vertex {
    glm::vec3 pos{0.0};
    glm::vec3 normal{0.0};
    glm::vec2 texCoord{0.0};

    static std::vector<VertexAttributeDescription> getAttributeDescriptions() {
        return {
            {.offset = offsetof(Vertex, pos), .format = vk::Format::eR32G32B32Sfloat},
            {.offset = offsetof(Vertex, normal), .format = vk::Format::eR32G32B32Sfloat},
            {.offset = offsetof(Vertex, texCoord), .format = vk::Format::eR32G32Sfloat},
        };
    }

    bool operator==(const Vertex& other) const {
        return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
    }
};

struct SphereMeshCreateInfo {
    int numSlices;
    int numStacks;
    float radius = 1.0;
};

struct CubeMeshCreateInfo {};

struct CubeLineMeshCreateInfo {};

struct PlaneMeshCreateInfo {
    float width = 1;
    float height = 1;
    uint32_t widthSegments = 1;
    uint32_t heightSegments = 1;
};

struct PlaneLineMeshCreateInfo {
    float width = 1;
    float height = 1;
    uint32_t widthSegments = 1;
    uint32_t heightSegments = 1;
};

struct Mesh {
    Mesh() = default;
    Mesh(const Context& context,
         const std::vector<Vertex>& vertices,
         const std::vector<uint32_t>& indices);
    static Mesh createSphereMesh(const Context& context, SphereMeshCreateInfo createInfo);
    static Mesh createPlaneMesh(const Context& context, PlaneMeshCreateInfo createInfo);
    static Mesh createPlaneLineMesh(const Context& context, PlaneLineMeshCreateInfo createInfo);
    static Mesh createCubeMesh(const Context& context, CubeMeshCreateInfo createInfo);
    static Mesh createCubeLineMesh(const Context& context, CubeLineMeshCreateInfo createInfo);

    uint32_t getIndicesCount() const { return indices.size(); }
    uint32_t getTriangleCount() const { return indices.size() / 3; }

    void drawIndexed(vk::CommandBuffer commandBuffer, uint32_t instanceCount) const {
        vk::DeviceSize offsets{0};
        commandBuffer.bindVertexBuffers(0, vertexBuffer->getBuffer(), offsets);
        commandBuffer.bindIndexBuffer(indexBuffer->getBuffer(), 0, vk::IndexType::eUint32);
        commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), instanceCount, 0, 0, 0);
    }

    const Context* context;
    BufferHandle vertexBuffer;
    BufferHandle indexBuffer;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
}  // namespace rv

namespace std {
template <>
struct hash<rv::Vertex> {
    size_t operator()(const rv::Vertex& vertex) const {
        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
               (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
};
}  // namespace std
