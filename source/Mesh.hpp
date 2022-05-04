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
    glm::vec3 normal;
    glm::vec2 texCoord;
};

class Mesh
{
public:
    void Init(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    void Init(const std::string& filepath);
    const Accel& GetAccel() const { return bottomAccel; }
    uint64_t GetVertexBufferAddress() const { return vertexBuffer.GetAddress(); }
    uint64_t GetIndexBufferAddress() const { return indexBuffer.GetAddress(); }

private:
    Buffer vertexBuffer;
    Buffer indexBuffer;
    Accel bottomAccel;
};
