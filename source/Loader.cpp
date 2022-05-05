#include <filesystem>
#include "Loader.hpp"
#include <spdlog/spdlog.h>

void Loader::LoadShape(const tinyobj::attrib_t& attrib, const tinyobj::shape_t& shape,
                       std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
{
    for (const auto& index : shape.mesh.indices) {
        Vertex vertex{};

        vertex.pos = {
            attrib.vertices[3 * index.vertex_index + 0],
           -attrib.vertices[3 * index.vertex_index + 1],
            attrib.vertices[3 * index.vertex_index + 2],
        };

        vertex.normal = {
            attrib.normals[3 * index.normal_index + 0],
           -attrib.normals[3 * index.normal_index + 1],
            attrib.normals[3 * index.normal_index + 2],
        };

        vertex.texCoord = {
            attrib.texcoords[2 * index.texcoord_index + 0],
            1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
        };

        vertices.push_back(vertex);
        indices.push_back(indices.size());
    }
}

// Load as single mesh
void Loader::LoadFromFile(const std::string& filepath,
                          std::vector<Vertex>& vertices,
                          std::vector<uint32_t>& indices)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::string dir = std::filesystem::path{ filepath }.parent_path().string();
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str(), dir.c_str())) {
        throw std::runtime_error(warn + err);
    }

    for (const auto& shape : shapes) {
        LoadShape(attrib, shape, vertices, indices);
    }
}

// Load as multiple mesh
void Loader::LoadFromFile(const std::string& filepath, std::vector<std::shared_ptr<Mesh>>& meshes)
{
    spdlog::info("Load file: {}", filepath);
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::string dir = std::filesystem::path{ filepath }.parent_path().string();
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str(), dir.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::vector<Material> mats(materials.size());
    for (size_t i = 0; i < materials.size(); i++) {
        mats[i].Ambient = { materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2] };
        mats[i].Diffuse = { materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2] };
        mats[i].Specular = { materials[i].specular[0], materials[i].specular[1], materials[i].specular[2] };
        mats[i].Emission = { materials[i].emission[0], materials[i].emission[1], materials[i].emission[2] };
    }

    for (const auto& shape : shapes) {
        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};
        LoadShape(attrib, shape, vertices, indices);
        int matIndex = shape.mesh.material_ids[0];
        meshes.push_back(std::make_shared<Mesh>(vertices, indices, mats[matIndex]));
    }
}
