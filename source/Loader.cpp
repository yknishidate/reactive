#include <string>
#include <algorithm>
#include <filesystem>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include "Loader.hpp"
#include "Vulkan/Image.hpp"

namespace
{
    void LoadShape(const tinyobj::attrib_t& attrib, const tinyobj::shape_t& shape,
                   std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
    {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
               -attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2],
            };

            if (attrib.normals.size() < 1) {
                throw std::runtime_error("This shape dosen't have normals.");
            }
            vertex.normal = {
                attrib.normals[3 * index.normal_index + 0],
               -attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2],
            };

            if (attrib.texcoords.size() < 1) {
                throw std::runtime_error("This shape dosen't have texcoords.");
            }
            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
            };

            vertices.push_back(vertex);
            indices.push_back(indices.size());
        }
    }
}

// Load as single mesh
void Loader::LoadFromFile(const std::string& filepath,
                          std::vector<Vertex>& vertices,
                          std::vector<uint32_t>& indices)
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

void Loader::LoadFromFile(const std::string& filepath,
                          std::vector<std::shared_ptr<Mesh>>& meshes,
                          std::vector<Image>& textures)
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

    int texCount = 0;
    std::unordered_map<std::string, int> textureNames{};

    std::vector<Material> mats(materials.size());
    for (size_t i = 0; i < materials.size(); i++) {
        mats[i].Ambient = { materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2] };
        mats[i].Diffuse = { materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2] };
        mats[i].Specular = { materials[i].specular[0], materials[i].specular[1], materials[i].specular[2] };
        mats[i].Emission = { materials[i].emission[0], materials[i].emission[1], materials[i].emission[2] };

        // diffuse
        if (!materials[i].diffuse_texname.empty()) {
            if (textureNames.contains(materials[i].diffuse_texname)) {
                mats[i].DiffuseTexture = textureNames[materials[i].diffuse_texname];
            } else {
                mats[i].DiffuseTexture = texCount;
                textureNames[materials[i].diffuse_texname] = texCount;
                texCount++;
            }
        }
        // specular
        if (!materials[i].specular_texname.empty()) {
            if (textureNames.contains(materials[i].specular_texname)) {
                mats[i].SpecularTexture = textureNames[materials[i].specular_texname];
            } else {
                mats[i].SpecularTexture = texCount;
                textureNames[materials[i].specular_texname] = texCount;
                texCount++;
            }
        }
        // alpha
        if (!materials[i].alpha_texname.empty()) {
            if (textureNames.contains(materials[i].alpha_texname)) {
                mats[i].AlphaTexture = textureNames[materials[i].alpha_texname];
            } else {
                mats[i].AlphaTexture = texCount;
                textureNames[materials[i].alpha_texname] = texCount;
                texCount++;
            }
        }
    }

    // load texture
    textures.resize(texCount);
    for (auto&& [name, index] : textureNames) {
        std::string path = name;
        std::replace(path.begin(), path.end(), '\\', '/');
        path = dir + "/" + path;
        spdlog::info("  texture {}: {}", index, path);
        textures[index].Init(path);
    }

    for (const auto& shape : shapes) {
        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};
        LoadShape(attrib, shape, vertices, indices);
        int matIndex = shape.mesh.material_ids[0];
        if (matIndex >= 0) {
            meshes.push_back(std::make_shared<Mesh>(vertices, indices, mats[matIndex]));
        } else {
            meshes.push_back(std::make_shared<Mesh>(vertices, indices));
        }
    }
}
