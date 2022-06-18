#include "Vulkan/Vulkan.hpp"
#include "Mesh.hpp"
#include "Loader.hpp"
#include <spdlog/spdlog.h>

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, Material material)
{
    this->material = material;
    this->vertices = vertices;
    this->indices = indices;
    vk::BufferUsageFlags usage{
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
        vk::BufferUsageFlagBits::eStorageBuffer |
        vk::BufferUsageFlagBits::eShaderDeviceAddress };

    vertexBuffer = DeviceBuffer{ usage, vertices };
    indexBuffer = DeviceBuffer{ usage, indices };
    bottomAccel = BottomAccel{ vertexBuffer, indexBuffer, vertices.size(), indices.size() / 3, vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation };
}

Mesh::Mesh(const std::string& filepath)
{
    Loader::LoadFromFile(filepath, vertices, indices);

    vk::BufferUsageFlags usage{
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
        vk::BufferUsageFlagBits::eStorageBuffer |
        vk::BufferUsageFlagBits::eShaderDeviceAddress };

    vertexBuffer = DeviceBuffer{ usage, vertices };
    indexBuffer = DeviceBuffer{ usage, indices };
    bottomAccel = BottomAccel{ vertexBuffer, indexBuffer, vertices.size(), indices.size() / 3,
                               vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation };
}

void Mesh::SetMaterial(const Material& material)
{
    this->material = material;
}
