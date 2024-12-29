#pragma once

#include <vector>

#include "../Graphics/Buffer.hpp"
#include "../math.hpp"

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

enum class MeshUsage {
    Graphics,
    RayTracing,
    Hybrid,
};

struct SphereMeshCreateInfo {
    int numSlices = 32;
    int numStacks = 32;
    float radius = 1.0;
    MeshUsage usage = MeshUsage::Graphics;
    std::string name = "Sphere";
};

struct CubeMeshCreateInfo {
    MeshUsage usage = MeshUsage::Graphics;
    std::string name = "Cube";
};

struct CubeLineMeshCreateInfo {
    MeshUsage usage = MeshUsage::Graphics;
    std::string name = "CubeLine";
};

struct PlaneMeshCreateInfo {
    float width = 1.0f;
    float height = 1.0f;
    uint32_t widthSegments = 1u;
    uint32_t heightSegments = 1u;
    MeshUsage usage = MeshUsage::Graphics;
    std::string name = "Plane";
};

struct PlaneLineMeshCreateInfo {
    float width = 1;
    float height = 1;
    uint32_t widthSegments = 1;
    uint32_t heightSegments = 1;
    MeshUsage usage = MeshUsage::Graphics;
    std::string name = "PlaneLine";
};

class Mesh {
public:
    Mesh() = default;
    Mesh(const Context& context,
         MeshUsage usage,
         vk::MemoryPropertyFlags memoryProps,
         std::vector<Vertex> vertices,
         std::vector<uint32_t> indices,
         std::string name);

    static auto createSphereMesh(const Context& context, SphereMeshCreateInfo createInfo) -> Mesh;
    static auto createPlaneMesh(const Context& context, PlaneMeshCreateInfo createInfo) -> Mesh;
    static auto createCubeMesh(const Context& context, CubeMeshCreateInfo createInfo) -> Mesh;
    static auto createPlaneLineMesh(const Context& context, PlaneLineMeshCreateInfo createInfo)
        -> Mesh;
    static auto createCubeLineMesh(const Context& context, CubeLineMeshCreateInfo createInfo)
        -> Mesh;

    auto getVertexBuffer() const -> const BufferHandle& { return m_vertexBuffer; }
    auto getIndexBuffer() const -> const BufferHandle& { return m_indexBuffer; }
    auto getVertexCount() const -> uint32_t { return static_cast<uint32_t>(m_vertices.size()); }
    auto getIndicesCount() const -> uint32_t { return static_cast<uint32_t>(m_indices.size()); }
    auto getTriangleCount() const -> uint32_t { return static_cast<uint32_t>(m_indices.size() / 3); }

private:
    const Context* m_context = nullptr;

    std::string m_name;

    BufferHandle m_vertexBuffer;
    BufferHandle m_indexBuffer;

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
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
