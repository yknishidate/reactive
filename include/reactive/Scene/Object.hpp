#pragma once
#include <memory>
#include <vulkan/vulkan.hpp>
#include "Mesh.hpp"

namespace rv {
using MeshHandle = std::shared_ptr<Mesh>;

struct Transform {
    auto getMatrix() const -> glm::mat4;
    auto getNormalMatrix() const -> glm::mat3;
    auto getVkMatrix() const -> vk::TransformMatrixKHR;

    glm::vec3 position{0};
    glm::vec3 scale{1};
    glm::quat rotation{1, 0, 0, 0};
};

struct ObjectData {
    glm::mat4 matrix{1.0};
    glm::mat4 normalMatrix{1.0};

    glm::vec4 diffuse{1.0};
    glm::vec4 emission{0.0};
    glm::vec4 specular{0.0};

    int diffuseTexture{-1};
    int alphaTexture{-1};
};

class Object {
public:
    explicit Object(const MeshHandle& mesh);
    virtual void update(float dt) {}

    auto getMesh() const -> MeshHandle { return mesh; }
    auto getTransform() const -> Transform { return transform; }

private:
    MeshHandle mesh;

    Transform transform{};
};
}  // namespace rv
