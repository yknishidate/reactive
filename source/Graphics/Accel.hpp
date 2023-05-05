#pragma once
#include "ArrayProxy.hpp"
#include "Buffer.hpp"
// #include "Geometry.hpp"
//
// class Mesh;
// class Object;

struct BottomAccelCreateInfo {
    const Mesh* mesh;
    vk::GeometryFlagsKHR geometryFlags = vk::GeometryFlagBitsKHR::eOpaque;
    vk::BuildAccelerationStructureFlagsKHR buildFlags =
        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    vk::AccelerationStructureBuildTypeKHR buildType =
        vk::AccelerationStructureBuildTypeKHR::eDevice;
};

struct TopAccelCreateInfo {
    ArrayProxy<std::pair<const BottomAccel*, glm::mat4>> bottomAccels;
    vk::GeometryFlagsKHR geometryFlags = vk::GeometryFlagBitsKHR::eOpaque;
    vk::BuildAccelerationStructureFlagsKHR buildFlags =
        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
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

    //     void rebuild(const ArrayProxy<Object>& objects);

    vk::AccelerationStructureKHR getAccel() const { return *accel; }
    vk::WriteDescriptorSetAccelerationStructureKHR getInfo() const { return {*accel}; }

private:
    const Context* context;
    vk::UniqueAccelerationStructureKHR accel;
    DeviceBuffer buffer;
    DeviceBuffer instanceBuffer;
    //     InstancesGeometry geometry;
};
