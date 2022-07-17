#pragma once
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>
#include "Mesh.hpp"

struct Transform
{
    glm::vec3 position = { 0.0f, 0.0f, 0.0f };
    glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
    glm::quat rotation = { 1.0f, 0.0f, 0.0f, 0.0f };

    glm::mat4 GetMatrix() const;
    glm::mat3 GetNormalMatrix() const;
    vk::TransformMatrixKHR GetVkMatrix() const;
};

struct ObjectData
{
    glm::mat4 matrix{ 1.0 };
    glm::mat4 normalMatrix{ 1.0 };
    glm::vec4 diffuse{ 1.0 };
    glm::vec4 emission{ 0.0 };
    glm::vec4 specular{ 0.0 };
    int diffuseTexture{ -1 };
    int alphaTexture{ -1 };
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
