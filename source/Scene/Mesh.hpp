#pragma once
#include "Graphics/Buffer.hpp"
#include "Graphics/Accel.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

struct Vertex
{
    glm::vec3 pos{ 0.0 };
    glm::vec3 normal{ 0.0 };
    glm::vec2 texCoord{ 0.0 };

    static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions()
    {
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
};

using Index = uint32_t;

struct Material
{
    glm::vec3 ambient{ 1.0 };
    glm::vec3 diffuse{ 1.0 };
    glm::vec3 specular{ 0.0 };
    glm::vec3 emission{ 0.0 };
    float shininess = 0.0;
    float ior = 1.0;
    int ambientTexture = -1;
    int diffuseTexture = -1;
    int specularTexture = -1;
    int alphaTexture = -1;
};

class Mesh
{
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, Material material = {});
    Mesh(const std::string& filepath);

    const BottomAccel& getAccel() const { return bottomAccel; }

    auto getVertexBuffer() const { return vertexBuffer.getBuffer(); }
    auto getIndexBuffer() const { return indexBuffer.getBuffer(); }
    uint64_t getVertexBufferAddress() const { return vertexBuffer.getAddress(); }
    uint64_t getIndexBufferAddress() const { return indexBuffer.getAddress(); }
    const std::vector<Vertex>& getVertices() const { return vertices; }
    uint32_t getTriangleCount() const { return indices.size() / 3; }
    vk::AccelerationStructureGeometryTrianglesDataKHR getTriangles() const;

    void drawIndexed(vk::CommandBuffer commandBuffer) const
    {
        vk::DeviceSize offsets{ 0 };
        commandBuffer.bindVertexBuffers(0, getVertexBuffer(), offsets);
        commandBuffer.bindIndexBuffer(getIndexBuffer(), 0, vk::IndexType::eUint32);
        commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    }

    Material getMaterial() const { return material; }
    void setMaterial(const Material& material);

private:
    DeviceBuffer vertexBuffer;
    DeviceBuffer indexBuffer;
    BottomAccel bottomAccel;
    Material material{};
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
