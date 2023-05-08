#include "Mesh.hpp"

Mesh::Mesh(const Context* context, MeshCreateInfo createInfo)
    : context{context}, vertices{createInfo.vertices}, indices{createInfo.indices} {
    vertexBuffer = context->createDeviceBuffer({
        .usage = BufferUsage::Vertex,
        .size = sizeof(Vertex) * vertices.size(),
        .data = vertices.data(),
    });
    indexBuffer = context->createDeviceBuffer({
        .usage = BufferUsage::Index,
        .size = sizeof(uint32_t) * indices.size(),
        .data = indices.data(),
    });
}

Mesh::Mesh(const Context* context, SphereMeshCreateInfo createInfo) : context{context} {
    // add top vertex
    vertices.push_back({{0, 1, 0}});
    uint32_t v0 = 0;

    int n_stacks = createInfo.numStacks;
    int n_slices = createInfo.numSlices;
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

    vertexBuffer = context->createDeviceBuffer({
        .usage = BufferUsage::Vertex,
        .size = sizeof(Vertex) * vertices.size(),
        .data = vertices.data(),
    });
    indexBuffer = context->createDeviceBuffer({
        .usage = BufferUsage::Index,
        .size = sizeof(uint32_t) * indices.size(),
        .data = indices.data(),
    });
}

Mesh::Mesh(const Context* context, PlaneMeshCreateInfo createInfo) : context{context} {
    vertices = std::vector<Vertex>{
        {glm::vec3(-1.0, 0.0, -1.0)},
        {glm::vec3(1.0, 0.0, -1.0)},
        {glm::vec3(1.0, 0.0, 1.0)},
        {glm::vec3(-1.0, 0.0, 1.0)},
    };
    indices = std::vector<uint32_t>{
        0, 1, 2, 0, 2, 3,
    };

    vertexBuffer = context->createDeviceBuffer({
        .usage = BufferUsage::Vertex,
        .size = sizeof(Vertex) * vertices.size(),
        .data = vertices.data(),
    });
    indexBuffer = context->createDeviceBuffer({
        .usage = BufferUsage::Index,
        .size = sizeof(uint32_t) * indices.size(),
        .data = indices.data(),
    });
}

Mesh::Mesh(const Context* context, CubeMeshCreateInfo createInfo) : context{context} {
    vertices = std::vector<Vertex>{
        {glm::vec3(-1.0, -1.0, -1.0)}, {glm::vec3(1.0, -1.0, -1.0)}, {glm::vec3(1.0, -1.0, 1.0)},
        {glm::vec3(-1.0, -1.0, 1.0)},  {glm::vec3(-1.0, 1.0, -1.0)}, {glm::vec3(1.0, 1.0, -1.0)},
        {glm::vec3(1.0, 1.0, 1.0)},    {glm::vec3(-1.0, 1.0, 1.0)},
    };
    indices = std::vector<uint32_t>{
        0, 1, 2,  // Top
        0, 2, 3,  // Top
        2, 6, 7,  // Front
        2, 7, 3,  // Front
        3, 7, 4,  // Left
        3, 4, 0,  // Left
        0, 4, 5,  // Back
        0, 5, 1,  // Back
        1, 5, 6,  // Right
        1, 6, 2,  // Right
        5, 4, 7,  // Bottom
        5, 7, 6,  // Bottom
    };

    vertexBuffer = context->createDeviceBuffer({
        .usage = BufferUsage::Vertex,
        .size = sizeof(Vertex) * vertices.size(),
        .data = vertices.data(),
    });
    indexBuffer = context->createDeviceBuffer({
        .usage = BufferUsage::Index,
        .size = sizeof(uint32_t) * indices.size(),
        .data = indices.data(),
    });
}

Mesh::Mesh(const Context* context, CubeLineMeshCreateInfo createInfo) : context{context} {
    vertices = std::vector<Vertex>{
        {glm::vec3(-1.0, -1.0, -1.0)}, {glm::vec3(1.0, -1.0, -1.0)}, {glm::vec3(1.0, -1.0, 1.0)},
        {glm::vec3(-1.0, -1.0, 1.0)},  {glm::vec3(-1.0, 1.0, -1.0)}, {glm::vec3(1.0, 1.0, -1.0)},
        {glm::vec3(1.0, 1.0, 1.0)},    {glm::vec3(-1.0, 1.0, 1.0)},
    };
    indices = std::vector<uint32_t>{0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6,
                                    6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};

    vertexBuffer = context->createDeviceBuffer({
        .usage = BufferUsage::Vertex,
        .size = sizeof(Vertex) * vertices.size(),
        .data = vertices.data(),
    });
    indexBuffer = context->createDeviceBuffer({
        .usage = BufferUsage::Index,
        .size = sizeof(uint32_t) * indices.size(),
        .data = indices.data(),
    });
}

vk::AccelerationStructureGeometryTrianglesDataKHR Mesh::getTrianglesData() const {
    vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData;
    trianglesData.setVertexFormat(vk::Format::eR32G32B32Sfloat);
    trianglesData.setVertexData(vertexBuffer.getAddress());
    trianglesData.setVertexStride(sizeof(Vertex));
    trianglesData.setMaxVertex(vertices.size());
    trianglesData.setIndexType(vk::IndexType::eUint32);
    trianglesData.setIndexData(indexBuffer.getAddress());
    return trianglesData;
}
