#pragma once
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>
#include "Mesh.hpp"

struct Transform
{
    glm::vec3 Position{ 0 };
    glm::vec3 Scale{ 1 };
    glm::quat Rotation{ 1, 0, 0, 0 };

    glm::mat4 GetMatrix() const;
    glm::mat3 GetNormalMatrix() const;
    vk::TransformMatrixKHR GetVkMatrix() const;
};

class Object
{
public:
    void Init(std::shared_ptr<Mesh> mesh);
    const Mesh& GetMesh() const;
    const Transform& GetTransform() const { return transform; }
    Transform& GetTransform() { return transform; }
    virtual void Update(float dt) {}

    Material GetMaterial();
    void SetMaterial(const Material& material);

private:
    std::shared_ptr<Mesh> mesh;
    Transform transform{};
    std::optional<Material> material;
};
