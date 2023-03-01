#pragma once
#include "Graphics/Accel.hpp"
#include "Graphics/Buffer.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

struct Vertex {
    glm::vec3 pos{0.0};
    glm::vec3 normal{0.0};
    glm::vec2 texCoord{0.0};

    static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions;
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

struct Material {
    glm::vec3 ambient{1.0};
    glm::vec3 diffuse{1.0};
    glm::vec3 specular{0.0};
    glm::vec3 emission{0.0};
    float shininess = 0.0;
    float ior = 1.0;
    int ambientTexture = -1;
    int diffuseTexture = -1;
    int specularTexture = -1;
    int alphaTexture = -1;
    int emissionTexture = -1;
};

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, int materialID);

    const BottomAccel& getAccel() const { return bottomAccel; }

    const auto& getVertexBuffer() const { return vertexBuffer; }
    const auto& getIndexBuffer() const { return indexBuffer; }
    uint64_t getVertexBufferAddress() const { return vertexBuffer.getAddress(); }
    uint64_t getIndexBufferAddress() const { return indexBuffer.getAddress(); }
    const std::vector<Vertex>& getVertices() const { return vertices; }
    uint32_t getIndicesCount() const { return indices.size(); }
    uint32_t getTriangleCount() const { return indices.size() / 3; }
    vk::AccelerationStructureGeometryTrianglesDataKHR getTriangles() const;

    void drawIndexed(vk::CommandBuffer commandBuffer) const {
        vk::DeviceSize offsets{0};
        commandBuffer.bindVertexBuffers(0, vertexBuffer.getBuffer(), offsets);
        commandBuffer.bindIndexBuffer(indexBuffer.getBuffer(), 0, vk::IndexType::eUint32);
        commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    }

    int getMaterialID() const { return materialID; }

    static Mesh createFrustumLines() {
        std::vector<Vertex> vertices{
            {glm::vec3(-1.0, -1.0, -1.0)}, {glm::vec3(1.0, -1.0, -1.0)},
            {glm::vec3(1.0, -1.0, 1.0)},   {glm::vec3(-1.0, -1.0, 1.0)},
            {glm::vec3(-1.0, 1.0, -1.0)},  {glm::vec3(1.0, 1.0, -1.0)},
            {glm::vec3(1.0, 1.0, 1.0)},    {glm::vec3(-1.0, 1.0, 1.0)},
        };
        std::vector<uint32_t> indices{0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6,
                                      6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};
        return {vertices, indices, 0};
    }

private:
    DeviceBuffer vertexBuffer;
    DeviceBuffer indexBuffer;
    BottomAccel bottomAccel;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    int materialID = -1;
};
