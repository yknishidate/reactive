#pragma once
#include <vector>
#include <memory>
#include "Mesh.hpp"
#include "Camera.hpp"
#include "Object.hpp"

class Image;

struct BoundingBox
{
    glm::vec3 min{ std::numeric_limits<float>::max() };
    glm::vec3 max{ -std::numeric_limits<float>::max() };
};

class Scene
{
public:
    Scene() = default;
    explicit Scene(const std::string& filepath,
                   glm::vec3 position = glm::vec3{ 0.0f },
                   glm::vec3 scale = glm::vec3{ 1.0f },
                   glm::vec3 rotation = glm::vec3{ 0.0f });
    void Setup();
    void Update(float dt);
    std::shared_ptr<Mesh>& AddMesh(const std::string& filepath);
    Object& AddObject(std::shared_ptr<Mesh> mesh);
    PointLight& AddPointLight(glm::vec3 intensity, glm::vec3 position);
    SphereLight& AddSphereLight(glm::vec3 intensity, glm::vec3 position, float radius);

    const auto& GetAccel() const { return topAccel; }
    const auto& GetTextures() const { return textures; }
    const auto& GetAddressBuffer() const { return addressBuffer; }
    auto& GetCamera() { return camera; }
    auto& GetObjects() { return objects; }
    auto GetBoundingBox() const { return bbox; }
    auto GetCenter() const { return (bbox.min + bbox.max) / 2.0f; }

private:
    std::vector<std::shared_ptr<Mesh>> meshes;
    std::vector<Image> textures;

    std::vector<Object> objects;
    std::vector<ObjectData> objectData;

    BoundingBox bbox;

    TopAccel topAccel;
    Camera camera;

    StorageBuffer objectBuffer;
    StorageBuffer addressBuffer;

    std::vector<PointLight> pointLights;
    std::vector<SphereLight> sphereLights;
    StorageBuffer pointLightBuffer;
    StorageBuffer sphereLightBuffer;
};
