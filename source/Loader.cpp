#include "Loader.hpp"

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

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
        throw std::runtime_error(warn + err);
    }

    for (const auto& shape : shapes) {
        LoadShape(attrib, shape, vertices, indices);
    }
}

// Load as multiple mesh
void Loader::LoadFromFile(const std::string& filepath, std::vector<std::shared_ptr<Mesh>>& meshes)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
        throw std::runtime_error(warn + err);
    }

    for (const auto& shape : shapes) {
        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};
        LoadShape(attrib, shape, vertices, indices);
        meshes.push_back(std::make_shared<Mesh>(vertices, indices));
    }
}