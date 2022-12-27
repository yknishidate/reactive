#include "Mesh.hpp"
#include "Loader.hpp"

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, Material material)
{
    this->material = material;
    this->vertices = vertices;
    this->indices = indices;
    vk::BufferUsageFlags usage{
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
        vk::BufferUsageFlagBits::eStorageBuffer |
        vk::BufferUsageFlagBits::eShaderDeviceAddress };

    vertexBuffer = DeviceBuffer{ usage | vk::BufferUsageFlagBits::eVertexBuffer, vertices };
    indexBuffer = DeviceBuffer{ usage | vk::BufferUsageFlagBits::eIndexBuffer, indices };
    bottomAccel = BottomAccel{ *this, vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation };
}

Mesh::Mesh(const std::string& filepath)
{
    Loader::loadFromFile(ASSET_DIR + filepath, vertices, indices);

    vk::BufferUsageFlags usage{
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
        vk::BufferUsageFlagBits::eStorageBuffer |
        vk::BufferUsageFlagBits::eShaderDeviceAddress };

    vertexBuffer = DeviceBuffer{ usage | vk::BufferUsageFlagBits::eVertexBuffer, vertices };
    indexBuffer = DeviceBuffer{ usage | vk::BufferUsageFlagBits::eIndexBuffer, indices };
    bottomAccel = BottomAccel{ *this, vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation };
}

vk::AccelerationStructureGeometryTrianglesDataKHR Mesh::getTriangles() const
{
    return vk::AccelerationStructureGeometryTrianglesDataKHR()
        .setVertexFormat(vk::Format::eR32G32B32Sfloat)
        .setVertexData(vertexBuffer.getAddress())
        .setVertexStride(sizeof(Vertex))
        .setMaxVertex(vertices.size())
        .setIndexType(vk::IndexType::eUint32)
        .setIndexData(indexBuffer.getAddress());
}

void Mesh::setMaterial(const Material& material)
{
    this->material = material;
}
