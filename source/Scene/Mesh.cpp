#include "Mesh.hpp"

Mesh::Mesh(const Context* context,
           const std::vector<Vertex>& vertices,
           const std::vector<uint32_t>& indices)
    : vertices{vertices}, indices{indices} {
    vertexBuffer = DeviceBuffer{context, BufferUsage::Vertex, vertices};
    indexBuffer = DeviceBuffer{context, BufferUsage::Index, indices};
}

Mesh Mesh::createCubeLines(const Context* context) {
    std::vector<Vertex> vertices{
        {glm::vec3(-1.0, -1.0, -1.0)}, {glm::vec3(1.0, -1.0, -1.0)}, {glm::vec3(1.0, -1.0, 1.0)},
        {glm::vec3(-1.0, -1.0, 1.0)},  {glm::vec3(-1.0, 1.0, -1.0)}, {glm::vec3(1.0, 1.0, -1.0)},
        {glm::vec3(1.0, 1.0, 1.0)},    {glm::vec3(-1.0, 1.0, 1.0)},
    };
    std::vector<uint32_t> indices{0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6,
                                  6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};
    return {context, vertices, indices};
}

Mesh Mesh::createSpherePolys(const Context* context, int n_slices, int n_stacks) {
    // add top vertex
    std::vector<Vertex> vertices;
    vertices.push_back({{0, 1, 0}});
    uint32_t v0 = 0;

    // generate vertices per stack / slice
    constexpr double PI = glm::pi<double>();
    for (int i = 0; i < n_stacks - 1; i++) {
        auto phi = PI * static_cast<double>(i + 1) / static_cast<double>(n_stacks);
        for (int j = 0; j < n_slices; j++) {
            auto theta = 2.0 * PI * static_cast<double>(j) / static_cast<double>(n_slices);
            auto x = std::sin(phi) * std::cos(theta);
            auto y = std::cos(phi);
            auto z = std::sin(phi) * std::sin(theta);
            glm::vec3 pos = {x, y, z};
            vertices.push_back({pos, glm::normalize(pos)});
        }
    }

    // add bottom vertex
    vertices.push_back({{0, -1, 0}});
    uint32_t v1 = vertices.size() - 1;

    // add top / bottom triangles
    std::vector<uint32_t> indices;
    for (int i = 0; i < n_slices; ++i) {
        auto i0 = i + 1;
        auto i1 = (i + 1) % n_slices + 1;
        indices.push_back(v0);
        indices.push_back(i0);
        indices.push_back(i1);
        i0 = i + n_slices * (n_stacks - 2) + 1;
        i1 = (i + 1) % n_slices + n_slices * (n_stacks - 2) + 1;
        indices.push_back(v1);
        indices.push_back(i0);
        indices.push_back(i1);
    }

    // add quads per stack / slice
    for (int j = 0; j < n_stacks - 2; j++) {
        auto j0 = j * n_slices + 1;
        auto j1 = (j + 1) * n_slices + 1;
        for (int i = 0; i < n_slices; i++) {
            auto i0 = j0 + i;
            auto i1 = j0 + (i + 1) % n_slices;
            auto i2 = j1 + (i + 1) % n_slices;
            auto i3 = j1 + i;
            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i2);
            indices.push_back(i3);
            indices.push_back(i0);
        }
    }
    return {context, vertices, indices};
}
