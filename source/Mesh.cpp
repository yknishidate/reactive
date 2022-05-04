#include "Vulkan.hpp"
#include "Mesh.hpp"
#include <tiny_obj_loader.h>

namespace
{
    void LoadFromFile(const std::string& filepath,
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
    }
}

Mesh::Mesh(const std::string& filepath)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    LoadFromFile(filepath, vertices, indices);

    vk::BufferUsageFlags usage{
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress
    };

    vertexBuffer.InitOnHost(sizeof(Vertex) * vertices.size(), usage);
    indexBuffer.InitOnHost(sizeof(uint32_t) * indices.size(), usage);
    vertexBuffer.Copy(vertices.data());
    indexBuffer.Copy(indices.data());

    bottomAccel.InitAsBottom(vertexBuffer, indexBuffer, vertices.size(), indices.size() / 3);
}
