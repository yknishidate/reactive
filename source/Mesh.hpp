#pragma once
#include "Vulkan/Buffer.hpp"
#include "Vulkan/Accel.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "share/structure.inc"

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

    const Accel& GetAccel() const { return *bottomAccel; }

    uint64_t GetVertexBufferAddress() const { return vertexBuffer.GetAddress(); }
    uint64_t GetIndexBufferAddress() const { return indexBuffer.GetAddress(); }
    const std::vector<Vertex>& GetVertices() const { return vertices; }
    const std::vector<uint32_t>& GetIndices() const { return indices; }

    Material GetMaterial() const { return material; }
    void SetMaterial(const Material& material);

private:
    DeviceBuffer vertexBuffer{};
    DeviceBuffer indexBuffer{};
    std::unique_ptr<BottomAccel> bottomAccel;
    Material material{};
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
