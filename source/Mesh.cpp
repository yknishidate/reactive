#include "Vulkan/Vulkan.hpp"
#include "Mesh.hpp"
#include "Loader.hpp"
#include <spdlog/spdlog.h>

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, Material material)
{
    this->material = material;
    vk::BufferUsageFlags usage{
    vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
    vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress
    };

    vertexBuffer.Init(usage, vertices);
    indexBuffer.Init(usage, indices);
    bottomAccel.Init(vertexBuffer, indexBuffer, vertices.size(), indices.size() / 3, vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation);
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

    vertexBuffer.Init(usage, vertices);
    indexBuffer.Init(usage, indices);
    bottomAccel.Init(vertexBuffer, indexBuffer, vertices.size(), indices.size() / 3,
                     vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation);
}
