#include "Mesh.hpp"

Mesh::Mesh(const Context* context,
           const std::vector<Vertex>& vertices,
           const std::vector<uint32_t>& indices)
    : vertices{vertices}, indices{indices} {
    vertexBuffer = DeviceBuffer{context, BufferUsage::Vertex, vertices};
    indexBuffer = DeviceBuffer{context, BufferUsage::Index, indices};
}
