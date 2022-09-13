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

    const BottomAccel& GetAccel() const { return bottomAccel; }

    auto GetVertexBuffer() const { return vertexBuffer.GetBuffer(); }
    auto GetIndexBuffer() const { return indexBuffer.GetBuffer(); }
    uint64_t GetVertexBufferAddress() const { return vertexBuffer.GetAddress(); }
    uint64_t GetIndexBufferAddress() const { return indexBuffer.GetAddress(); }
    const std::vector<Vertex>& GetVertices() const { return vertices; }
    uint32_t GetTriangleCount() const { return indices.size() / 3; }
    vk::AccelerationStructureGeometryTrianglesDataKHR GetTriangles() const;

    Material GetMaterial() const { return material; }
    void SetMaterial(const Material& material);

private:
    DeviceBuffer vertexBuffer;
    DeviceBuffer indexBuffer;
    BottomAccel bottomAccel;
    Material material{};
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
