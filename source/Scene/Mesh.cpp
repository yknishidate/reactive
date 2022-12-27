#include "Mesh.hpp"
#include "Loader.hpp"

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, Material material)
{
    this->material = material;
    this->vertices = vertices;
    this->indices = indices;
    vertexBuffer = DeviceBuffer{ BufferUsage::Vertex, vertices };
    indexBuffer = DeviceBuffer{ BufferUsage::Index, indices };
    bottomAccel = BottomAccel{ *this, vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation };
}

Mesh::Mesh(const std::string& filepath)
{
    Loader::loadFromFile(ASSET_DIR + filepath, vertices, indices);

    vertexBuffer = DeviceBuffer{ BufferUsage::Vertex, vertices };
    indexBuffer = DeviceBuffer{ BufferUsage::Index, indices };
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
