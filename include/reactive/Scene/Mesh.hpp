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

    auto operator==(const Vertex& other) const -> bool {
        return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
    }

    static auto getAttributeDescriptions() -> std::vector<VertexAttributeDescription>;
};

struct SphereMeshCreateInfo {
    int numSlices = 32;
    int numStacks = 32;
    float radius = 1.0;
    std::string name = "Sphere";
};

struct CubeMeshCreateInfo {
    std::string name = "Cube";
};

struct CubeLineMeshCreateInfo {
    std::string name = "CubeLine";
};

struct PlaneMeshCreateInfo {
    float width = 1;
    float height = 1;
    uint32_t widthSegments = 1;
    uint32_t heightSegments = 1;
    std::string name = "Plane";
};

struct PlaneLineMeshCreateInfo {
    float width = 1;
    float height = 1;
    uint32_t widthSegments = 1;
    uint32_t heightSegments = 1;
    std::string name = "PlaneLine";
};

struct Mesh {
    Mesh() = default;
    Mesh(const Context& context,
         const std::vector<Vertex>& vertices,
         const std::vector<uint32_t>& indices,
         const std::string& name);
    static auto createSphereMesh(const Context& context, SphereMeshCreateInfo createInfo) -> Mesh;
    static auto createPlaneMesh(const Context& context, PlaneMeshCreateInfo createInfo) -> Mesh;
    static auto createCubeMesh(const Context& context, CubeMeshCreateInfo createInfo) -> Mesh;
    static auto createPlaneLineMesh(const Context& context, PlaneLineMeshCreateInfo createInfo)
        -> Mesh;
    static auto createCubeLineMesh(const Context& context, CubeLineMeshCreateInfo createInfo)
        -> Mesh;

    auto getVertexCount() const -> uint32_t { return vertices.size(); }
    auto getIndicesCount() const -> uint32_t { return indices.size(); }
    auto getTriangleCount() const -> uint32_t { return indices.size() / 3; }

    const Context* context;

    std::string name;

    BufferHandle vertexBuffer;
    BufferHandle indexBuffer;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
}  // namespace rv

namespace std {
template <>
struct hash<rv::Vertex> {
    auto operator()(const rv::Vertex& vertex) const -> size_t {
        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
               (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
};
}  // namespace std
