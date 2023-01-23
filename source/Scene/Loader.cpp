#include "Loader.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <filesystem>
#include <set>
#include <string>
#include <unordered_map>
#include "Graphics/Image.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace {
void LoadShape(const tinyobj::attrib_t& attrib,
               const tinyobj::shape_t& shape,
               std::vector<Vertex>& vertices,
               std::vector<uint32_t>& indices) {
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
}  // namespace

// Load as single mesh
void Loader::loadFromFile(const std::string& filepath,
                          std::vector<Vertex>& vertices,
                          std::vector<uint32_t>& indices) {
    spdlog::info("Load file: {}", filepath);
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::string dir = std::filesystem::path{filepath}.parent_path().string();
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str(),
                          dir.c_str())) {
        throw std::runtime_error(warn + err);
    }

    for (const auto& shape : shapes) {
        LoadShape(attrib, shape, vertices, indices);
    }
}

// Load as multiple mesh
void Loader::loadFromFile(const std::string& filepath, std::vector<Mesh>& meshes) {
    spdlog::info("Load file: {}", filepath);
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::string dir = std::filesystem::path{filepath}.parent_path().string();
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str(),
                          dir.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::vector<Material> mats(materials.size());
    for (size_t i = 0; i < materials.size(); i++) {
        auto& mat = materials[i];
        mats[i].ambient = {mat.ambient[0], mat.ambient[1], mat.ambient[2]};
        mats[i].diffuse = {mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]};
        mats[i].specular = {mat.specular[0], mat.specular[1], mat.specular[2]};
        mats[i].emission = {mat.emission[0], mat.emission[1], mat.emission[2]};
    }

    meshes.reserve(shapes.size());
    for (const auto& shape : shapes) {
        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};
        LoadShape(attrib, shape, vertices, indices);
        int matIndex = shape.mesh.material_ids[0];
        meshes.emplace_back(vertices, indices, mats[matIndex]);
    }
}

void Loader::loadFromFile(const std::string& filepath,
                          std::vector<Mesh>& meshes,
                          std::vector<Image>& textures) {
    spdlog::info("Load file: {}", filepath);
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::string dir = std::filesystem::path{filepath}.parent_path().string();
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str(),
                          dir.c_str())) {
        spdlog::error("Failed to load: {}", warn + err);
    }
    spdlog::info("Shapes: {}", shapes.size());
    spdlog::info("Materials: {}", materials.size());

    int texCount = 0;
    std::unordered_map<std::string, int> textureNames{};

    std::vector<Material> mats(materials.size());
    for (size_t i = 0; i < materials.size(); i++) {
        auto& mat = materials[i];
        mats[i].ambient = {mat.ambient[0], mat.ambient[1], mat.ambient[2]};
        mats[i].diffuse = {mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]};
        mats[i].specular = {mat.specular[0], mat.specular[1], mat.specular[2]};
        mats[i].emission = {mat.emission[0], mat.emission[1], mat.emission[2]};

        // diffuse
        if (!mat.diffuse_texname.empty()) {
            if (textureNames.contains(mat.diffuse_texname)) {
                mats[i].diffuseTexture = textureNames[mat.diffuse_texname];
            } else {
                mats[i].diffuseTexture = texCount;
                textureNames[mat.diffuse_texname] = texCount;
                texCount++;
            }
        }
        // specular
        if (!mat.specular_texname.empty()) {
            if (textureNames.contains(mat.specular_texname)) {
                mats[i].specularTexture = textureNames[mat.specular_texname];
            } else {
                mats[i].specularTexture = texCount;
                textureNames[mat.specular_texname] = texCount;
                texCount++;
            }
        }
        // alpha
        if (!mat.alpha_texname.empty()) {
            if (textureNames.contains(mat.alpha_texname)) {
                mats[i].alphaTexture = textureNames[mat.alpha_texname];
            } else {
                mats[i].alphaTexture = texCount;
                textureNames[mat.alpha_texname] = texCount;
                texCount++;
            }
        }
    }

    // load texture
    textures = std::vector<Image>(texCount);
    for (auto& [name, index] : textureNames) {
        std::string path = name;
        std::replace(path.begin(), path.end(), '\\', '/');
        path = dir + "/" + path;
        spdlog::info("  Texture {}: {}", index, path);
        textures[index] = Image{path};
    }

    for (const auto& shape : shapes) {
        spdlog::info("  Shape {}", shape.name);

        // remove object which has multiple materials
        std::set<int> indexSet;
        for (auto& i : shape.mesh.material_ids) {
            indexSet.insert(i);
        }
        if (indexSet.size() != 1) {
            continue;
        }

        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};
        LoadShape(attrib, shape, vertices, indices);
        int matIndex = shape.mesh.material_ids[0];

        meshes.reserve(shapes.size());
        if (matIndex >= 0) {
            meshes.emplace_back(vertices, indices, mats[matIndex]);
        } else {
            meshes.emplace_back(vertices, indices);
        }
    }
}
