#include "Object.hpp"

glm::mat4 Transform::getMatrix() const
{
    return glm::translate(position) * glm::toMat4(rotation) * glm::scale(scale);
}

glm::mat3 Transform::getNormalMatrix() const
{
    return glm::toMat3(rotation);
}

vk::TransformMatrixKHR Transform::getVkMatrix() const
{
    glm::mat4 transposed = glm::transpose(getMatrix());
    std::array<std::array<float, 4>, 3> data;
    std::memcpy(&data, &transposed, sizeof(vk::TransformMatrixKHR));
    return vk::TransformMatrixKHR{ data };
}

//Object::Object(std::shared_ptr<Mesh> mesh)
//    : mesh{ mesh }
//{
//}

Object::Object(const Mesh& mesh)
    : mesh{ &mesh }
{
}


const Mesh& Object::getMesh() const
{
    assert(mesh && "Mesh is nullptr");
    return *mesh;
}

Material Object::getMaterial()
{
    if (material) {
        return material.value();
    }
    return mesh->getMaterial();
}

void Object::setMaterial(const Material& material)
{
    this->material = material;
}
