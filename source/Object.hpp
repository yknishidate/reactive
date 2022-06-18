#pragma once
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>
#include "Mesh.hpp"

struct Transform
{
    glm::vec3 position{ 0 };
    glm::vec3 scale{ 1 };
    glm::quat rotation{ 1, 0, 0, 0 };

    glm::mat4 GetMatrix() const;
    glm::mat3 GetNormalMatrix() const;
    vk::TransformMatrixKHR GetVkMatrix() const;
};

class Object
{
public:
    explicit Object(std::shared_ptr<Mesh> mesh);
    virtual void Update(float dt) {}

    const Mesh& GetMesh() const;

    Material GetMaterial();
    void SetMaterial(const Material& material);

    Transform transform{};

private:
    std::shared_ptr<Mesh> mesh;
    std::optional<Material> material;
};
