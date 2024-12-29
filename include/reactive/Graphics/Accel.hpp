#pragma once
#include "ArrayProxy.hpp"
#include "Buffer.hpp"
#include "../math.hpp"

namespace rv {
struct BottomAccelCreateInfo {
    uint32_t vertexStride;
    uint32_t maxVertexCount;
    uint32_t maxTriangleCount;

    vk::GeometryFlagsKHR geometryFlags = vk::GeometryFlagBitsKHR::eOpaque;

    vk::BuildAccelerationStructureFlagsKHR buildFlags =
        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace |
        vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;

    vk::AccelerationStructureBuildTypeKHR buildType =
        vk::AccelerationStructureBuildTypeKHR::eDevice;

    std::string debugName;
};

struct AccelInstance {
    BottomAccelHandle bottomAccel;

    glm::mat4 transform = glm::mat4{1.0};

    uint32_t sbtOffset = 0;

    uint32_t customIndex = 0;
};

inline vk::TransformMatrixKHR toVkMatrix(const glm::mat4& matrix) {
    const glm::mat4 transposedMatrix = glm::transpose(matrix);

    vk::TransformMatrixKHR vkMatrix;
    std::memcpy(&vkMatrix, &transposedMatrix, sizeof(vk::TransformMatrixKHR));

    return vkMatrix;
}

struct TopAccelCreateInfo {
    ArrayProxy<AccelInstance> accelInstances;

    vk::GeometryFlagsKHR geometryFlags = vk::GeometryFlagBitsKHR::eOpaque;

    vk::BuildAccelerationStructureFlagsKHR buildFlags =
        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace |
        vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;

    vk::AccelerationStructureBuildTypeKHR buildType =
        vk::AccelerationStructureBuildTypeKHR::eDevice;

    std::string debugName;
};

class BottomAccel {
    friend class CommandBuffer;

public:
    BottomAccel(const Context& context, const BottomAccelCreateInfo& createInfo);

    auto getBufferAddress() const -> uint64_t { return m_buffer->getAddress(); }

private:
    const Context* m_context;

    vk::UniqueAccelerationStructureKHR m_accel;

    BufferHandle m_buffer;
    BufferHandle m_scratchBuffer;

    vk::AccelerationStructureGeometryTrianglesDataKHR m_trianglesData;
    vk::GeometryFlagsKHR m_geometryFlags;
    vk::BuildAccelerationStructureFlagsKHR m_buildFlags;
    vk::AccelerationStructureBuildTypeKHR m_buildType;

    uint32_t m_maxPrimitiveCount;
};

class TopAccel {
    friend class CommandBuffer;

public:
    TopAccel(const Context& context, const TopAccelCreateInfo& createInfo);

    auto getAccel() const -> vk::AccelerationStructureKHR { return *m_accel; }

    auto getInfo() const -> vk::WriteDescriptorSetAccelerationStructureKHR { return {*m_accel}; }

    void updateInstances(ArrayProxy<AccelInstance> accelInstances) const;

private:
    const Context* m_context;

    vk::UniqueAccelerationStructureKHR m_accel;

    BufferHandle m_buffer;
    BufferHandle m_instanceBuffer;
    BufferHandle m_scratchBuffer;

    uint32_t m_primitiveCount;
    vk::AccelerationStructureGeometryInstancesDataKHR m_instancesData;
    vk::GeometryFlagsKHR m_geometryFlags;
    vk::BuildAccelerationStructureFlagsKHR m_buildFlags;
    vk::AccelerationStructureBuildTypeKHR m_buildType;
};
}  // namespace rv
