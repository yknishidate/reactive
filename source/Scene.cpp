#include "Vulkan.hpp"
#include "Scene.hpp"
#include <tiny_obj_loader.h>

namespace
{
    // Load as multiple mesh
    void LoadFromFile(const std::string& filepath,
                      std::vector<std::shared_ptr<Mesh>>& meshes)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
            throw std::runtime_error(warn + err);
        }

        for (int meshIndex = 0; meshIndex < shapes.size(); meshIndex++) {

        }
    }
}

Scene::Scene(const std::string& filepath)
{

}
