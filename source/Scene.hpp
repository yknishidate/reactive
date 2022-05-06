#pragma once
#include <vector>
#include <memory>
#include "Mesh.hpp"
#include "Camera.hpp"
#include "Object.hpp"

class Image;

struct ObjectData
{
    glm::mat4 Matrix{ 1 };
    glm::mat4 NormalMatrix{ 1 };
    glm::vec4 Diffuse{ 1.0 };
    int TextureIndex = -1;
};

class Scene
{
public:
    explicit Scene(const std::string& filepath);
    void Setup();
    void Update(float dt);
    void ProcessInput();
    const std::shared_ptr<Mesh>& AddMesh(const std::string& filepath);
    const Object& AddObject(std::shared_ptr<Mesh> mesh);

    vk::AccelerationStructureKHR GetAccel() const { return topAccel.GetAccel(); }
    const std::vector<Image>& GetTextures() const { return textures; }
    const Buffer& GetAddressBuffer() const { return addressBuffer; }
    Camera& GetCamera() { return camera; }

private:
    std::vector<std::shared_ptr<Mesh>> meshes;
    std::vector<Image> textures;

    std::vector<Object> objects;
    std::vector<ObjectData> objectData;

    Accel topAccel;
    Camera camera;

    Buffer objectBuffer;
    Buffer addressBuffer;
};
