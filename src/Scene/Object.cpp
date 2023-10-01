#include "Scene/Object.hpp"

namespace rv {
auto rv::Transform::getMatrix() const -> glm::mat4 {
    return glm::translate(position) * glm::toMat4(rotation) * glm::scale(scale);
}

auto Transform::getNormalMatrix() const -> glm::mat3 {
    return glm::toMat3(rotation);
}

auto Transform::getVkMatrix() const -> vk::TransformMatrixKHR {
    glm::mat4 transposed = glm::transpose(getMatrix());
    std::array<std::array<float, 4>, 3> data;
    std::memcpy(&data, &transposed, sizeof(vk::TransformMatrixKHR));
    return vk::TransformMatrixKHR{data};
}
}  // namespace rv
