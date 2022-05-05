#include "Vulkan.hpp"
#include "Mesh.hpp"
#include "Loader.hpp"

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, int materialIndex)
{
    this->materialIndex = materialIndex;
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

Mesh::Mesh(const std::string& filepath)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    Loader::LoadFromFile(filepath, vertices, indices);

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
