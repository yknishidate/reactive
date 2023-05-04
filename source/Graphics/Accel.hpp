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

// class TopAccel {
// public:
//     TopAccel() = default;
//     explicit TopAccel(const ArrayProxy<Object>& objects,
//                       vk::GeometryFlagBitsKHR geometryFlag = vk::GeometryFlagBitsKHR::eOpaque);
//
//     void rebuild(const ArrayProxy<Object>& objects);
//
//     vk::AccelerationStructureKHR getAccel() const { return *accel; }
//     vk::WriteDescriptorSetAccelerationStructureKHR getInfo() const { return {*accel}; }
//
// private:
//     vk::UniqueAccelerationStructureKHR accel;
//     DeviceBuffer buffer;
//     DeviceBuffer instanceBuffer;
//     InstancesGeometry geometry;
// };
