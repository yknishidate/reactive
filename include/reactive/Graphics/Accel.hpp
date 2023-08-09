#pragma once
#include <glm/glm.hpp>
#include "ArrayProxy.hpp"
#include "Buffer.hpp"

namespace rv {
struct BottomAccelCreateInfo {
    const DeviceBuffer& vertexBuffer;
    const DeviceBuffer& indexBuffer;
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
    const BottomAccel& bottomAccel;
    glm::mat4 transform = glm::mat4{1.0};
    uint32_t sbtOffset = 0;
};

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
    BottomAccel() = default;
    BottomAccel(const Context* context, BottomAccelCreateInfo createInfo);

    uint64_t getBufferAddress() const { return buffer.getAddress(); }

private:
    const Context* context;
    vk::UniqueAccelerationStructureKHR accel;
    DeviceBuffer buffer;
};

class TopAccel {
public:
    TopAccel() = default;
    TopAccel(const Context* context, TopAccelCreateInfo createInfo);

    void update(vk::CommandBuffer commandBuffer, ArrayProxy<AccelInstance> accelInstances);

    vk::AccelerationStructureKHR getAccel() const { return *accel; }
    vk::WriteDescriptorSetAccelerationStructureKHR getInfo() const { return {*accel}; }

private:
    const Context* context;
    vk::UniqueAccelerationStructureKHR accel;
    DeviceBuffer buffer;
    DeviceBuffer instanceBuffer;
    HostBuffer scratchBuffer;

    vk::GeometryFlagsKHR geometryFlags;
    vk::BuildAccelerationStructureFlagsKHR buildFlags;
    vk::AccelerationStructureBuildTypeKHR buildType;
};
}  // namespace rv
