#include "Vulkan/Vulkan.hpp"
#include "Scene.hpp"
#include "Loader.hpp"
#include "Window.hpp"

Scene::Scene(const std::string& filepath,
             glm::vec3 position, glm::vec3 scale, glm::vec3 rotation)
{
    Loader::LoadFromFile(filepath, meshes, textures);

    objects.resize(meshes.size());
    for (int i = 0; i < meshes.size(); i++) {
        objects[i].Init(meshes[i]);
        objects[i].GetTransform().Position = position;
        objects[i].GetTransform().Scale = scale;
        objects[i].GetTransform().Rotation = glm::quat{ rotation };
        for (const auto& vertex : meshes[i]->GetVertices()) {
            glm::vec3 pos = objects[i].GetTransform().GetMatrix() * glm::vec4{ vertex.pos, 1.0 };
            bbox.min = glm::min(bbox.min, pos);
            bbox.max = glm::max(bbox.max, pos);
        }
    }
}

void Scene::Setup()
{
    topAccel.Init(objects, vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation);

    // Create object data
    for (auto&& object : objects) {
        ObjectData data;
        data.Matrix = object.GetTransform().GetMatrix();
        data.NormalMatrix = object.GetTransform().GetNormalMatrix();
        data.Diffuse = glm::vec4{ object.GetMaterial().Diffuse, 1 };
        data.Emission = glm::vec4{ object.GetMaterial().Emission, 1 };
        data.Specular = glm::vec4{ 0.0f };
        data.DiffuseTexture = object.GetMaterial().DiffuseTexture;
        data.AlphaTexture = object.GetMaterial().AlphaTexture;
        objectData.push_back(data);
    }
    vk::BufferUsageFlags usage =
        vk::BufferUsageFlagBits::eStorageBuffer |
        vk::BufferUsageFlagBits::eShaderDeviceAddress;
    objectBuffer.Init(usage, objectData);
    if (!pointLights.empty()) {
        pointLightBuffer.Init(usage, pointLights);
    }
    if (!sphereLights.empty()) {
        sphereLightBuffer.Init(usage, sphereLights);
    }

    // Buffer references
    std::vector<BufferAddress> addresses(objects.size());
    for (int i = 0; i < objects.size(); i++) {
        addresses[i].vertices = objects[i].GetMesh().GetVertexBufferAddress();
        addresses[i].indices = objects[i].GetMesh().GetIndexBufferAddress();
        addresses[i].objects = objectBuffer.GetAddress();
        if (!pointLights.empty()) {
            addresses[i].pointLights = pointLightBuffer.GetAddress();
        }
        if (!sphereLights.empty()) {
            addresses[i].sphereLights = sphereLightBuffer.GetAddress();
        }
    }
    addressBuffer.Init(usage, addresses);

    camera.Init(Window::GetWidth(), Window::GetHeight());
}

void Scene::Update(float dt)
{
    static float time = 0.0f;
    time += dt;
    for (int i = 0; i < objects.size(); i++) {
        objects[i].GetTransform().Rotation = glm::quat{ glm::vec3(0.0f, time * 0.1, 0.0f) };
        objectData[i].Matrix = objects[i].GetTransform().GetMatrix();
        objectData[i].NormalMatrix = objects[i].GetTransform().GetNormalMatrix();
    }
    objectBuffer.Copy(objectData.data());
    topAccel.Rebuild(objects);
}

void Scene::ProcessInput()
{
    camera.ProcessInput();
}

std::shared_ptr<Mesh>& Scene::AddMesh(const std::string& filepath)
{
    meshes.push_back(std::make_shared<Mesh>(filepath));
    return meshes.back();
}

Object& Scene::AddObject(std::shared_ptr<Mesh> mesh)
{
    Object object;
    object.Init(mesh);
    objects.push_back(object);
    return objects.back();
}

PointLight& Scene::AddPointLight(glm::vec3 intensity, glm::vec3 position)
{
    pointLights.emplace_back(intensity, position);
    return pointLights.back();
}

SphereLight& Scene::AddSphereLight(glm::vec3 intensity, glm::vec3 position, float radius)
{
    static bool added = false;
    static int sphereMeshIndex;
    if (!added) {
        Material lightMaterial;
        lightMaterial.Diffuse = glm::vec3{ 0.0f };
        lightMaterial.Emission = glm::vec3{ 1.0f };

        auto mesh = AddMesh("../asset/Sphere.obj");
        mesh->SetMaterial(lightMaterial);
        sphereMeshIndex = meshes.size() - 1;
    }
    added = true;

    Material lightMaterial;
    lightMaterial.Diffuse = glm::vec3{ 0.0f };
    lightMaterial.Emission = intensity;

    objects.push_back({});
    objects.back().Init(meshes[sphereMeshIndex]);
    objects.back().GetTransform().Position = position;
    objects.back().GetTransform().Scale = glm::vec3{ radius };
    objects.back().SetMaterial(lightMaterial);

    sphereLights.emplace_back(intensity, position, radius);
    return sphereLights.back();
}
