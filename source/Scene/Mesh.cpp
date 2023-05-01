#include "Mesh.hpp"

Mesh::Mesh(const Context* context,
           const std::vector<Vertex>& vertices,
           const std::vector<uint32_t>& indices)
    : m_vertices{vertices}, m_indices{indices} {
    m_vertexBuffer = DeviceBuffer{context, BufferUsage::Vertex, m_vertices};
    m_indexBuffer = DeviceBuffer{context, BufferUsage::Index, m_indices};
}
