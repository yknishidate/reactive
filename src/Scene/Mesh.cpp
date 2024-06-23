#include "Scene/Mesh.hpp"

#include "Graphics/CommandBuffer.hpp"

namespace rv {
auto Vertex::getAttributeDescriptions() -> std::vector<VertexAttributeDescription> {
    return {
        {.offset = offsetof(Vertex, pos), .format = vk::Format::eR32G32B32Sfloat},
        {.offset = offsetof(Vertex, normal), .format = vk::Format::eR32G32B32Sfloat},
        {.offset = offsetof(Vertex, texCoord), .format = vk::Format::eR32G32Sfloat},
    };
}

Mesh::Mesh(const Context& _context,
           MeshUsage usage,
           vk::MemoryPropertyFlags memoryProps,
           std::vector<Vertex> _vertices,
           std::vector<uint32_t> _indices,
           std::string _name)
    : context{&_context},
      name{std::move(_name)},
      vertices{std::move(_vertices)},
      indices{std::move(_indices)} {
    vk::BufferUsageFlags vertexUsage{};
    vk::BufferUsageFlags indexUsage{};
    if (usage == MeshUsage::Graphics) {
        vertexUsage = vk::BufferUsageFlagBits::eStorageBuffer |
                      vk::BufferUsageFlagBits::eShaderDeviceAddress |
                      vk::BufferUsageFlagBits::eVertexBuffer |
                      vk::BufferUsageFlagBits::eTransferDst;
        indexUsage = vk::BufferUsageFlagBits::eStorageBuffer |
                     vk::BufferUsageFlagBits::eShaderDeviceAddress |
                     vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    } else if (usage == MeshUsage::RayTracing) {
        vertexUsage = vk::BufferUsageFlagBits::eStorageBuffer |
                      vk::BufferUsageFlagBits::eShaderDeviceAddress |
                      vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                      vk::BufferUsageFlagBits::eTransferDst;
        indexUsage = vk::BufferUsageFlagBits::eStorageBuffer |
                     vk::BufferUsageFlagBits::eShaderDeviceAddress |
                     vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                     vk::BufferUsageFlagBits::eTransferDst;
    } else if (usage == MeshUsage::Hybrid) {
        vertexUsage = vk::BufferUsageFlagBits::eStorageBuffer |
                      vk::BufferUsageFlagBits::eShaderDeviceAddress |
                      vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                      vk::BufferUsageFlagBits::eVertexBuffer |
                      vk::BufferUsageFlagBits::eTransferDst;
        indexUsage = vk::BufferUsageFlagBits::eStorageBuffer |
                     vk::BufferUsageFlagBits::eShaderDeviceAddress |
                     vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                     vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    }

    vertexBuffer = context->createBuffer({
        .usage = vertexUsage,
        .memory = memoryProps,
        .size = sizeof(Vertex) * vertices.size(),
        .debugName = name + "::vertexBuffer",
    });

    indexBuffer = context->createBuffer({
        .usage = indexUsage,
        .memory = memoryProps,
        .size = sizeof(uint32_t) * indices.size(),
        .debugName = name + "::indexBuffer",
    });

    if (memoryProps & vk::MemoryPropertyFlagBits::eHostVisible) {
        vertexBuffer->copy(vertices.data());
        indexBuffer->copy(indices.data());
    } else {
        context->oneTimeSubmit([&](CommandBufferHandle commandBuffer) {
            commandBuffer->copyBuffer(vertexBuffer, vertices.data());
            commandBuffer->copyBuffer(indexBuffer, indices.data());
        });
    }
}

auto Mesh::createSphereMesh(const Context& context, SphereMeshCreateInfo createInfo) -> Mesh {
    int n_stacks = createInfo.numStacks;
    int n_slices = createInfo.numSlices;
    float radius = createInfo.radius;

    // add top vertex
    std::vector<Vertex> vertices{};
    std::vector<uint32_t> indices{};
    vertices.push_back({{0, radius, 0}, {0, 1, 0}});
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
    vertices.push_back({{0, -radius, 0}, {0, -1, 0}});
    uint32_t v1 = static_cast<uint32_t>(vertices.size()) - 1;

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

    return {context, createInfo.usage, MemoryUsage::Device, vertices, indices, createInfo.name};
}

auto Mesh::createPlaneMesh(const Context& context, PlaneMeshCreateInfo createInfo) -> Mesh {
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

    return {context, createInfo.usage, MemoryUsage::Device, vertices, indices, createInfo.name};
}

auto Mesh::createPlaneLineMesh(const Context& context, PlaneLineMeshCreateInfo createInfo) -> Mesh {
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
    return {context, createInfo.usage, MemoryUsage::Device, vertices, indices, createInfo.name};
}

auto Mesh::createCubeMesh(const Context& context, CubeMeshCreateInfo createInfo) -> Mesh {
    // Y-up, Right-hand
    glm::vec3 v0 = {-1, -1, -1};
    glm::vec3 v1 = {+1, -1, -1};
    glm::vec3 v2 = {-1, +1, -1};
    glm::vec3 v3 = {+1, +1, -1};
    glm::vec3 v4 = {-1, -1, +1};
    glm::vec3 v5 = {+1, -1, +1};
    glm::vec3 v6 = {-1, +1, +1};
    glm::vec3 v7 = {+1, +1, +1};

    glm::vec3 pX = {+1, 0, 0};
    glm::vec3 nX = {-1, 0, 0};
    glm::vec3 pY = {0, +1, 0};
    glm::vec3 nY = {0, -1, 0};
    glm::vec3 pZ = {0, 0, +1};
    glm::vec3 nZ = {0, 0, -1};
    //       2           3
    //       +-----------+
    //      /|          /|
    //    /  |        /  |
    //  6+---+-------+7  |
    //   |  0+-------+---+1
    //   |  /        |  /
    //   |/          |/
    //  4+-----------+5

    std::vector<Vertex> vertices = {
        {v0, nZ}, {v2, nZ}, {v1, nZ},  // Back
        {v3, nZ}, {v1, nZ}, {v2, nZ},  // Back
        {v4, pZ}, {v5, pZ}, {v6, pZ},  // Front
        {v7, pZ}, {v6, pZ}, {v5, pZ},  // Front
        {v6, pY}, {v7, pY}, {v2, pY},  // Top
        {v3, pY}, {v2, pY}, {v7, pY},  // Top
        {v0, nY}, {v1, nY}, {v4, nY},  // Bottom
        {v5, nY}, {v4, nY}, {v1, nY},  // Bottom
        {v5, pX}, {v1, pX}, {v7, pX},  // Right
        {v3, pX}, {v7, pX}, {v1, pX},  // Right
        {v0, nX}, {v4, nX}, {v2, nX},  // Left
        {v6, nX}, {v2, nX}, {v4, nX},  // Left
    };

    std::vector<uint32_t> indices;
    for (int i = 0; i < vertices.size(); i++) {
        indices.push_back(i);
    }
    return {context, createInfo.usage, MemoryUsage::Device, vertices, indices, createInfo.name};
}

auto Mesh::createCubeLineMesh(const Context& context, CubeLineMeshCreateInfo createInfo) -> Mesh {
    std::vector<Vertex> vertices = {
        {glm::vec3(-1.0, -1.0, -1.0)}, {glm::vec3(1.0, -1.0, -1.0)}, {glm::vec3(1.0, -1.0, 1.0)},
        {glm::vec3(-1.0, -1.0, 1.0)},  {glm::vec3(-1.0, 1.0, -1.0)}, {glm::vec3(1.0, 1.0, -1.0)},
        {glm::vec3(1.0, 1.0, 1.0)},    {glm::vec3(-1.0, 1.0, 1.0)},
    };
    std::vector<uint32_t> indices = {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6,
                                     6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};
    return {context, createInfo.usage, MemoryUsage::Device, vertices, indices, createInfo.name};
}
}  // namespace rv
