#pragma once
#include <vulkan/vulkan.hpp>
#include "Mesh.hpp"

struct Transform
{
    glm::vec3 Position{ 0 };
    glm::vec3 Scale{ 1 };
    glm::quat Rotation{ 1, 0, 0, 0 };

    const glm::mat4& GetMatrix();
    const glm::mat3& GetNormalMatrix();
    const vk::TransformMatrixKHR& GetVkMatrix();
};

class Object
{
public:
    const Mesh& GetMesh() const { return mesh; }
    const Transform& GetTransform() const { return transform; }

private:
    Mesh& mesh;
    Transform transform;
};
