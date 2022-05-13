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
        objects[i].transform.position = position;
        objects[i].transform.scale = scale;
        objects[i].transform.rotation = glm::quat{ rotation };
        for (const auto& vertex : meshes[i]->GetVertices()) {
            glm::vec3 pos = objects[i].transform.GetMatrix() * glm::vec4{ vertex.pos, 1.0 };
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
        data.matrix = object.transform.GetMatrix();
        data.normalMatrix = object.transform.GetNormalMatrix();
        data.diffuse = glm::vec4{ object.GetMaterial().diffuse, 1 };
        data.emission = glm::vec4{ object.GetMaterial().emission, 1 };
        data.specular = glm::vec4{ 0.0f };
        data.diffuseTexture = object.GetMaterial().diffuseTexture;
        data.alphaTexture = object.GetMaterial().alphaTexture;
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
        objects[i].transform.rotation = glm::quat{ glm::vec3(0.0f, time * 0.1, 0.0f) };
        objectData[i].matrix = objects[i].transform.GetMatrix();
        objectData[i].normalMatrix = objects[i].transform.GetNormalMatrix();
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
        lightMaterial.emission = glm::vec3{ 1.0f };

        auto mesh = AddMesh("../asset/Sphere.obj");
        mesh->SetMaterial(lightMaterial);
        sphereMeshIndex = meshes.size() - 1;
    }
    added = true;

    Material lightMaterial;
    lightMaterial.emission = intensity;

    objects.push_back({});
    objects.back().Init(meshes[sphereMeshIndex]);
    objects.back().transform.position = position;
    objects.back().transform.scale = glm::vec3{ radius };
    objects.back().SetMaterial(lightMaterial);

    sphereLights.emplace_back(intensity, position, radius);
    return sphereLights.back();
}
