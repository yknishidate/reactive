#include "Vulkan/Vulkan.hpp"
#include "Vulkan/Buffer.hpp"
#include "Object.hpp"

glm::mat4 Transform::GetMatrix() const
{
    return glm::translate(position) * glm::toMat4(rotation) * glm::scale(scale);
}

glm::mat3 Transform::GetNormalMatrix() const
{
    return glm::toMat3(rotation);
}

vk::TransformMatrixKHR Transform::GetVkMatrix() const
{
    glm::mat4 transposed = glm::transpose(GetMatrix());
    std::array<std::array<float, 4>, 3> data;
    std::memcpy(&data, &transposed, sizeof(vk::TransformMatrixKHR));
    return vk::TransformMatrixKHR{ data };
}

Object::Object(std::shared_ptr<Mesh> mesh)
    : mesh{ mesh }
{
}

const Mesh& Object::GetMesh() const
{
    assert(mesh && "Mesh is nullptr");
    return *mesh;
}

Material Object::GetMaterial()
{
    if (material) {
        return material.value();
    }
    return mesh->GetMaterial();
}

void Object::SetMaterial(const Material& material)
{
    this->material = material;
}
