#include "Mesh.hpp"

Mesh::Mesh(const App* app,
           const std::vector<Vertex>& vertices,
           const std::vector<uint32_t>& indices)
    : m_vertices{vertices}, m_indices{indices} {
    m_vertexBuffer = DeviceBuffer{app, BufferUsage::Vertex, m_vertices};
    m_indexBuffer = DeviceBuffer{app, BufferUsage::Index, m_indices};
}
