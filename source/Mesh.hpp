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

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct Material
{
    glm::vec3 Ambient{ 1.0 };
    glm::vec3 Diffuse{ 1.0 };
    glm::vec3 Specular{ 0.0 };
    glm::vec3 Emission{ 0.0 };
    float Shininess = 0.0;
    float IOR = 1.0;
    int AmbientTexture = -1;
    int DiffuseTexture = -1;
    int SpecularTexture = -1;
    int AlphaTexture = -1;
};

class Mesh
{
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, Material material = {});
    Mesh(const std::string& filepath);
    const Accel& GetAccel() const { return bottomAccel; }
    uint64_t GetVertexBufferAddress() const { return vertexBuffer.GetAddress(); }
    uint64_t GetIndexBufferAddress() const { return indexBuffer.GetAddress(); }
    Material GetMaterial() const { return material; }
    void SetMaterial(const Material& material);

private:
    DeviceBuffer vertexBuffer{};
    DeviceBuffer indexBuffer{};
    BottomAccel bottomAccel{};
    Material material{};
};
