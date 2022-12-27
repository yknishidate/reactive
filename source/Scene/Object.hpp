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

    glm::mat4 getMatrix() const;
    glm::mat3 getNormalMatrix() const;
    vk::TransformMatrixKHR getVkMatrix() const;
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
    explicit Object(const Mesh& mesh);
    virtual void Update(float dt) {}

    const Mesh& getMesh() const;

    Material getMaterial();
    void setMaterial(const Material& material);

    Transform transform{};

private:
    const Mesh* mesh;
    std::optional<Material> material;
};
