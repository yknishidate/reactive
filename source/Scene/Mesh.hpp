#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include "App.hpp"
#include "Graphics/Buffer.hpp"

struct Vertex {
    glm::vec3 pos{0.0};
    glm::vec3 normal{0.0};
    glm::vec2 texCoord{0.0};

    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions(3);
        attributeDescriptions[0].setBinding(0);
        attributeDescriptions[0].setLocation(0);
        attributeDescriptions[0].setFormat(vk::Format::eR32G32B32Sfloat);
        attributeDescriptions[0].setOffset(offsetof(Vertex, pos));

        attributeDescriptions[1].setBinding(0);
        attributeDescriptions[1].setLocation(1);
        attributeDescriptions[1].setFormat(vk::Format::eR32G32B32Sfloat);
        attributeDescriptions[1].setOffset(offsetof(Vertex, normal));

        attributeDescriptions[2].setBinding(0);
        attributeDescriptions[2].setLocation(2);
        attributeDescriptions[2].setFormat(vk::Format::eR32G32Sfloat);
        attributeDescriptions[2].setOffset(offsetof(Vertex, texCoord));
        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const {
        return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
    }
};

namespace std {
template <>
struct hash<Vertex> {
    size_t operator()(const Vertex& vertex) const {
        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
               (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
};
}  // namespace std

using Index = uint32_t;

// struct Material {
//     glm::vec3 ambient{1.0};
//     glm::vec3 diffuse{1.0};
//     glm::vec3 specular{0.0};
//     glm::vec3 emission{0.0};
//     float shininess = 0.0;
//     float ior = 1.0;
//     int ambientTexture = -1;
//     int diffuseTexture = -1;
//     int specularTexture = -1;
//     int alphaTexture = -1;
//     int emissionTexture = -1;
// };

struct Mesh {
    Mesh() = default;
    Mesh(const Context* context,
         const std::vector<Vertex>& vertices,
         const std::vector<uint32_t>& indices);

    uint64_t getVertexBufferAddress() const { return vertexBuffer.getAddress(); }
    uint64_t getIndexBufferAddress() const { return indexBuffer.getAddress(); }
    uint32_t getIndicesCount() const { return indices.size(); }
    uint32_t getTriangleCount() const { return indices.size() / 3; }

    void drawIndexed(vk::CommandBuffer commandBuffer) const {
        vk::DeviceSize offsets{0};
        commandBuffer.bindVertexBuffers(0, vertexBuffer.getBuffer(), offsets);
        commandBuffer.bindIndexBuffer(indexBuffer.getBuffer(), 0, vk::IndexType::eUint32);
        commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    }

    static Mesh createCubeLines(const Context* context) {
        std::vector<Vertex> vertices{
            {glm::vec3(-1.0, -1.0, -1.0)}, {glm::vec3(1.0, -1.0, -1.0)},
            {glm::vec3(1.0, -1.0, 1.0)},   {glm::vec3(-1.0, -1.0, 1.0)},
            {glm::vec3(-1.0, 1.0, -1.0)},  {glm::vec3(1.0, 1.0, -1.0)},
            {glm::vec3(1.0, 1.0, 1.0)},    {glm::vec3(-1.0, 1.0, 1.0)},
        };
        std::vector<uint32_t> indices{0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6,
                                      6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};
        return {context, vertices, indices};
    }

    static Mesh createSpherePolys(const Context* context, int n_slices, int n_stacks) {
        // add top vertex
        std::vector<Vertex> vertices;
        vertices.push_back({{0, 1, 0}});
        uint32_t v0 = 0;

        // generate vertices per stack / slice
        constexpr double PI = glm::pi<double>();
        for (int i = 0; i < n_stacks - 1; i++) {
            auto phi = PI * static_cast<double>(i + 1) / static_cast<double>(n_stacks);
            for (int j = 0; j < n_slices; j++) {
                auto theta = 2.0 * PI * static_cast<double>(j) / static_cast<double>(n_slices);
                auto x = std::sin(phi) * std::cos(theta);
                auto y = std::cos(phi);
                auto z = std::sin(phi) * std::sin(theta);
                glm::vec3 pos = {x, y, z};
                vertices.push_back({pos, glm::normalize(pos)});
            }
        }

        // add bottom vertex
        vertices.push_back({{0, -1, 0}});
        uint32_t v1 = vertices.size() - 1;

        // add top / bottom triangles
        std::vector<uint32_t> indices;
        for (int i = 0; i < n_slices; ++i) {
            auto i0 = i + 1;
            auto i1 = (i + 1) % n_slices + 1;
            indices.push_back(v0);
            indices.push_back(i0);
            indices.push_back(i1);
            i0 = i + n_slices * (n_stacks - 2) + 1;
            i1 = (i + 1) % n_slices + n_slices * (n_stacks - 2) + 1;
            indices.push_back(v1);
            indices.push_back(i0);
            indices.push_back(i1);
        }

        // add quads per stack / slice
        for (int j = 0; j < n_stacks - 2; j++) {
            auto j0 = j * n_slices + 1;
            auto j1 = (j + 1) * n_slices + 1;
            for (int i = 0; i < n_slices; i++) {
                auto i0 = j0 + i;
                auto i1 = j0 + (i + 1) % n_slices;
                auto i2 = j1 + (i + 1) % n_slices;
                auto i3 = j1 + i;
                indices.push_back(i0);
                indices.push_back(i1);
                indices.push_back(i2);
                indices.push_back(i2);
                indices.push_back(i3);
                indices.push_back(i0);
            }
        }
        return {context, vertices, indices};
    }

    DeviceBuffer vertexBuffer;
    DeviceBuffer indexBuffer;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
