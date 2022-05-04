#include "Vulkan.hpp"
#include "Buffer.hpp"
#include "Object.hpp"

glm::mat4 Transform::GetMatrix() const
{
    return glm::translate(Position) * glm::toMat4(Rotation) * glm::scale(Scale);
}

glm::mat3 Transform::GetNormalMatrix() const
{
    return glm::toMat3(Rotation);
}

vk::TransformMatrixKHR Transform::GetVkMatrix() const
{
    glm::mat4 transposed = glm::transpose(GetMatrix());
    std::array<std::array<float, 4>, 3> data;
    std::memcpy(&data, &transposed, sizeof(vk::TransformMatrixKHR));
    return vk::TransformMatrixKHR{ data };
}

void Object::Init(std::shared_ptr<Mesh> mesh)
{
    this->mesh = mesh;
}

const Mesh& Object::GetMesh() const
{
    assert(mesh && "Mesh is nullptr");
    return *mesh;
}
