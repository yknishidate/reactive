#pragma once
#include "Buffer.hpp"
#include "Accel.hpp"

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
    //glm::vec3 normal;
    //glm::vec2 texCoord;
};

class Mesh
{
public:
    void Init(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    {
        vk::BufferUsageFlags usage{
            vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress
        };

        vertexBuffer.InitOnHost(sizeof(Vertex) * vertices.size(), usage);
        indexBuffer.InitOnHost(sizeof(uint32_t) * indices.size(), usage);
        vertexBuffer.Copy(vertices.data());
        indexBuffer.Copy(indices.data());

        bottomAccel.InitAsBottom(vertexBuffer, indexBuffer, sizeof(Vertex), vertices.size(), indices.size() / 3);
    }

    const Accel& GetAccel() const { return bottomAccel; }

private:
    Buffer vertexBuffer;
    Buffer indexBuffer;
    Accel bottomAccel;
};
