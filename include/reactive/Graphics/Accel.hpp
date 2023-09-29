#pragma once
#include <glm/glm.hpp>
#include "ArrayProxy.hpp"
#include "Buffer.hpp"

namespace rv {
struct BottomAccelCreateInfo {
    BufferHandle vertexBuffer;
    BufferHandle indexBuffer;
    uint32_t vertexStride;
    uint32_t vertexCount;
    uint32_t triangleCount;
    vk::GeometryFlagsKHR geometryFlags = vk::GeometryFlagBitsKHR::eOpaque;
    vk::BuildAccelerationStructureFlagsKHR buildFlags =
        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    vk::AccelerationStructureBuildTypeKHR buildType =
        vk::AccelerationStructureBuildTypeKHR::eDevice;
};

struct AccelInstance {
    BottomAccelHandle bottomAccel;
    glm::mat4 transform = glm::mat4{1.0};
    uint32_t sbtOffset = 0;
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
};

class BottomAccel {
public:
    BottomAccel(const Context* context, BottomAccelCreateInfo createInfo);

    uint64_t getBufferAddress() const { return buffer->getAddress(); }

private:
    const Context* context;
    vk::UniqueAccelerationStructureKHR accel;
    BufferHandle buffer;
};

class TopAccel {
    friend class CommandBuffer;

public:
    TopAccel(const Context* context, TopAccelCreateInfo createInfo);

    vk::AccelerationStructureKHR getAccel() const { return *accel; }
    vk::WriteDescriptorSetAccelerationStructureKHR getInfo() const { return {*accel}; }

private:
    const Context* context;
    vk::UniqueAccelerationStructureKHR accel;
    BufferHandle buffer;
    BufferHandle instanceBuffer;
    BufferHandle scratchBuffer;

    vk::GeometryFlagsKHR geometryFlags;
    vk::BuildAccelerationStructureFlagsKHR buildFlags;
    vk::AccelerationStructureBuildTypeKHR buildType;
};
}  // namespace rv
