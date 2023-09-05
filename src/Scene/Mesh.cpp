#include "Scene/Mesh.hpp"

namespace rv {
Mesh::Mesh(const Context& context,
           const std::vector<Vertex>& vertices,
           const std::vector<uint32_t>& indices)
    : context{&context}, vertices{vertices}, indices{indices} {
    vertexBuffer = context.createBuffer({
        .usage = BufferUsage::Vertex,
        .memory = MemoryUsage::Device,
        .size = sizeof(Vertex) * vertices.size(),
        .data = vertices.data(),
    });
    indexBuffer = context.createBuffer({
        .usage = BufferUsage::Index,
        .memory = MemoryUsage::Device,
        .size = sizeof(uint32_t) * indices.size(),
        .data = indices.data(),
    });
}

Mesh Mesh::createSphereMesh(const Context& context, SphereMeshCreateInfo createInfo) {
    int n_stacks = createInfo.numStacks;
    int n_slices = createInfo.numSlices;
    float radius = createInfo.radius;

    // add top vertex
    std::vector<Vertex> vertices{};
    std::vector<uint32_t> indices{};
    vertices.push_back({{0, radius, 0}});
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
            glm::vec3 pos = glm::vec3{x, y, z} * radius;
            vertices.push_back({pos, glm::normalize(pos)});
        }
    }

    // add bottom vertex
    vertices.push_back({{0, -radius, 0}});
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

    return {context, vertices, indices};
}

Mesh Mesh::createPlaneMesh(const Context& context, PlaneMeshCreateInfo createInfo) {
    float width = createInfo.width;
    float height = createInfo.height;
    uint32_t widthSegments = createInfo.widthSegments;
    uint32_t heightSegments = createInfo.heightSegments;
    uint32_t verticesCount = (widthSegments + 1) * (heightSegments + 1);
    uint32_t indicesCount = 3 * 2 * widthSegments * heightSegments;

    std::vector<Vertex> vertices{};
    std::vector<uint32_t> indices{};
    vertices.reserve(verticesCount);
    indices.reserve(indicesCount);
    for (uint32_t h = 0; h <= heightSegments; h++) {
        for (uint32_t w = 0; w <= widthSegments; w++) {
            float u = w / float(widthSegments);
            float v = h / float(heightSegments);
            vertices.push_back({
                .pos = glm::vec3((u - 0.5f) * width, 0.0f, (v - 0.5f) * height),
                .normal = glm::vec3(0, 1, 0),
                .texCoord = glm::vec2(u, v),
            });
        }
    }
    for (uint32_t h = 0; h < heightSegments; h++) {
        for (uint32_t w = 0; w < widthSegments; w++) {
            uint32_t i0 = h * (widthSegments + 1) + w;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + widthSegments + 1;
            uint32_t i3 = i0 + widthSegments + 2;
            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i3);
            indices.push_back(i3);
            indices.push_back(i2);
            indices.push_back(i0);
        }
    }

    return {context, vertices, indices};
}

Mesh Mesh::createPlaneLineMesh(const Context& context, PlaneLineMeshCreateInfo createInfo) {
    float width = createInfo.width;
    float height = createInfo.height;
    uint32_t widthSegments = createInfo.widthSegments;
    uint32_t heightSegments = createInfo.heightSegments;

    uint32_t verticesCount = 2 * (widthSegments + 1) + 2 * (heightSegments + 1);
    uint32_t indicesCount = verticesCount;

    std::vector<Vertex> vertices{};
    std::vector<uint32_t> indices{};
    vertices.reserve(verticesCount);
    indices.reserve(indicesCount);
    for (uint32_t h = 0; h <= heightSegments; h++) {
        float v = h / float(heightSegments);
        vertices.push_back({
            .pos = glm::vec3(-0.5f * width, 0.0f, (v - 0.5f) * height),
            .normal = glm::vec3(0, 1, 0),
            .texCoord = glm::vec2(0.0f, v),
        });
        vertices.push_back({
            .pos = glm::vec3(0.5f * width, 0.0f, (v - 0.5f) * height),
            .normal = glm::vec3(0, 1, 0),
            .texCoord = glm::vec2(1.0f, v),
        });
    }
    for (uint32_t w = 0; w <= widthSegments; w++) {
        float u = w / float(widthSegments);
        vertices.push_back({
            .pos = glm::vec3((u - 0.5f) * width, 0.0f, -0.5f * height),
            .normal = glm::vec3(0, 1, 0),
            .texCoord = glm::vec2(u, 0.0f),
        });
        vertices.push_back({
            .pos = glm::vec3((u - 0.5f) * width, 0.0f, 0.5f * height),
            .normal = glm::vec3(0, 1, 0),
            .texCoord = glm::vec2(u, 1.0f),
        });
    }

    for (uint32_t i = 0; i < indicesCount; i++) {
        indices.push_back(i);
    }
    return {context, vertices, indices};
}

Mesh Mesh::createCubeMesh(const Context& context, CubeMeshCreateInfo createInfo) {
    std::vector<glm::vec3> positions{
        {-1.0, -1.0, -1.0}, {1.0, -1.0, -1.0}, {1.0, -1.0, 1.0}, {-1.0, -1.0, 1.0},
        {-1.0, 1.0, -1.0},  {1.0, 1.0, -1.0},  {1.0, 1.0, 1.0},  {-1.0, 1.0, 1.0},
    };
    std::vector<glm::vec3> normals{
        {1, 0, 0},   // 0 +X
        {0, 1, 0},   // 1 +Y
        {0, 0, 1},   // 2 +Z
        {-1, 0, 0},  // 3 -X
        {0, -1, 0},  // 4 -Y
        {0, 0, -1},  // 5 -Z
    };
    //      3           2
    //      +-----------+
    //     /|         / |
    //   /  |        /  |
    // 0+---+-------+1  |
    //  |  7+-------+---+6
    //  |  /        |  /
    //  |/          |/
    // 4+-----------+5

    std::vector<Vertex> vertices = {
        {positions[0], normals[4]}, {positions[1], normals[4]},
        {positions[2], normals[4]},  // Top -Y
        {positions[0], normals[4]}, {positions[2], normals[4]},
        {positions[3], normals[4]},  // Top -Y
        {positions[2], normals[5]}, {positions[6], normals[5]},
        {positions[7], normals[5]},  // Front -Z
        {positions[2], normals[5]}, {positions[7], normals[5]},
        {positions[3], normals[5]},  // Front -Z
        {positions[3], normals[3]}, {positions[7], normals[3]},
        {positions[4], normals[3]},  // Left -X
        {positions[3], normals[3]}, {positions[4], normals[3]},
        {positions[0], normals[3]},  // Left -X
        {positions[0], normals[2]}, {positions[4], normals[2]},
        {positions[5], normals[2]},  // Back +Z
        {positions[0], normals[2]}, {positions[5], normals[2]},
        {positions[1], normals[2]},  // Back +Z
        {positions[1], normals[0]}, {positions[5], normals[0]},
        {positions[6], normals[0]},  // Right +X
        {positions[1], normals[0]}, {positions[6], normals[0]},
        {positions[2], normals[0]},  // Right +X
        {positions[5], normals[1]}, {positions[4], normals[1]},
        {positions[7], normals[1]},  // Bottom Y
        {positions[5], normals[1]}, {positions[7], normals[1]},
        {positions[6], normals[1]},  // Bottom Y
    };

    std::vector<uint32_t> indices;
    for (int i = 0; i < vertices.size(); i++) {
        indices.push_back(i);
    }
    return {context, vertices, indices};
}

Mesh Mesh::createCubeLineMesh(const Context& context, CubeLineMeshCreateInfo createInfo) {
    std::vector<Vertex> vertices = {
        {glm::vec3(-1.0, -1.0, -1.0)}, {glm::vec3(1.0, -1.0, -1.0)}, {glm::vec3(1.0, -1.0, 1.0)},
        {glm::vec3(-1.0, -1.0, 1.0)},  {glm::vec3(-1.0, 1.0, -1.0)}, {glm::vec3(1.0, 1.0, -1.0)},
        {glm::vec3(1.0, 1.0, 1.0)},    {glm::vec3(-1.0, 1.0, 1.0)},
    };
    std::vector<uint32_t> indices = {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6,
                                     6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};
    return {context, vertices, indices};
}
}  // namespace rv
