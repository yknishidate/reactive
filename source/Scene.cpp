#include "Scene.hpp"
#include "Loader.hpp"
#include "Window.hpp"

Scene::Scene(const std::string& filepath,
             glm::vec3 position, glm::vec3 scale, glm::vec3 rotation)
{
    Loader::LoadFromFile(filepath, meshes, textures);

    // If textures is empty, append dummy texture
    if (textures.empty()) {
        textures = std::vector<Image>(1);
        textures[0] = Image{ 1, 1, vk::Format::eR8G8B8A8Unorm };
    }

    // Create objects
    objects.reserve(meshes.size());
    for (int i = 0; i < meshes.size(); i++) {
        Object object(meshes[i]);
        object.transform.position = position;
        object.transform.scale = scale;
        object.transform.rotation = glm::quat{ rotation };
        for (const auto& vertex : meshes[i]->GetVertices()) {
            glm::vec3 pos = object.transform.GetMatrix() * glm::vec4{ vertex.pos, 1.0 };
            bbox.min = glm::min(bbox.min, pos);
            bbox.max = glm::max(bbox.max, pos);
        }
        objects.push_back(object);
    }
}

void Scene::Setup()
{
    topAccel = TopAccel{ objects };

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
    objectBuffer = StorageBuffer{ objectData };
    if (!pointLights.empty()) {
        pointLightBuffer = StorageBuffer{ pointLights };
    }
    if (!sphereLights.empty()) {
        sphereLightBuffer = StorageBuffer{ sphereLights };
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
    addressBuffer = StorageBuffer{ addresses };

    camera = Camera{ Window::GetWidth(), Window::GetHeight() };
}

void Scene::Update(float dt)
{
    camera.ProcessInput();
    //static float time = 0.0f;
    //time += dt;
    //for (int i = 0; i < objects.size(); i++) {
    //    objects[i].transform.rotation = glm::quat{ glm::vec3(0.0f, time * 0.1, 0.0f) };
    //    objectData[i].matrix = objects[i].transform.GetMatrix();
    //    objectData[i].normalMatrix = objects[i].transform.GetNormalMatrix();
    //}
    //objectBuffer.Copy(objectData.data());
    //topAccel.Rebuild(objects);
}

std::shared_ptr<Mesh>& Scene::AddMesh(const std::string& filepath)
{
    return meshes.emplace_back(std::make_shared<Mesh>(filepath));
}

Object& Scene::AddObject(std::shared_ptr<Mesh> mesh)
{
    return objects.emplace_back(mesh);
}

PointLight& Scene::AddPointLight(glm::vec3 intensity, glm::vec3 position)
{
    return pointLights.emplace_back(intensity, position);
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

    auto& obj = objects.emplace_back(meshes[sphereMeshIndex]);
    obj.transform.position = position;
    obj.transform.scale = glm::vec3{ radius };
    obj.SetMaterial(lightMaterial);

    return sphereLights.emplace_back(intensity, position, radius);
}
