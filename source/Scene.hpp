#pragma once
#include <vector>
#include <memory>
#include "Mesh.hpp"
#include "Camera.hpp"
#include "Object.hpp"
#include "Light.hpp"

class Image;

struct ObjectData
{
    glm::mat4 Matrix{ 1 };
    glm::mat4 NormalMatrix{ 1 };
    glm::vec4 Diffuse{ 1.0 };
    glm::vec4 Emission{ 0.0 };
    glm::vec4 Specular{ 0.0 };
    int TextureIndex = -1;
};

struct BufferAddress
{
    vk::DeviceAddress vertices;
    vk::DeviceAddress indices;
    vk::DeviceAddress objects;
    vk::DeviceAddress pointLights;
    vk::DeviceAddress sphereLights;
};

class Scene
{
public:
    explicit Scene(const std::string& filepath,
                   glm::vec3 position = glm::vec3{ 0.0f },
                   glm::vec3 scale = glm::vec3{ 1.0f },
                   glm::vec3 rotation = glm::vec3{ 0.0f });
    void Setup();
    void Update(float dt);
    void ProcessInput();
    std::shared_ptr<Mesh>& AddMesh(const std::string& filepath);
    Object& AddObject(std::shared_ptr<Mesh> mesh);
    PointLight& AddPointLight(glm::vec3 intensity, glm::vec3 position);
    SphereLight& AddSphereLight(glm::vec3 intensity, glm::vec3 position, float radius);

    vk::AccelerationStructureKHR GetAccel() const { return topAccel.GetAccel(); }
    const std::vector<Image>& GetTextures() const { return textures; }
    const Buffer& GetAddressBuffer() const { return addressBuffer; }
    Camera& GetCamera() { return camera; }
    std::vector<Object>& GetObjects() { return objects; }
    int GetNumPointLights() const { return pointLights.size(); }
    int GetNumSphereLights() const { return sphereLights.size(); }

private:
    std::vector<std::shared_ptr<Mesh>> meshes;
    std::vector<Image> textures;

    std::vector<Object> objects;
    std::vector<ObjectData> objectData;

    TopAccel topAccel;
    Camera camera;

    DeviceBuffer objectBuffer;
    DeviceBuffer addressBuffer;

    std::vector<PointLight> pointLights;
    std::vector<SphereLight> sphereLights;
    DeviceBuffer pointLightBuffer;
    DeviceBuffer sphereLightBuffer;
};
