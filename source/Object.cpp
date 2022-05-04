#include "Vulkan.hpp"
#include "Buffer.hpp"
#include "Object.hpp"

const glm::mat4& Transform::GetMatrix()
{
    return glm::translate(Position) * glm::toMat4(Rotation) * glm::scale(Scale);
}

const glm::mat3& Transform::GetNormalMatrix()
{
    return glm::toMat3(Rotation);
}

const vk::TransformMatrixKHR& Transform::GetVkMatrix()
{
    glm::mat4 transposed = glm::transpose(GetMatrix());
    std::array<std::array<float, 4>, 3> data;
    std::memcpy(&data, &transposed, sizeof(vk::TransformMatrixKHR));
    return vk::TransformMatrixKHR{ data };
}
