#pragma once
#include "ArrayProxy.hpp"
#include "Geometry.hpp"

class Mesh;
class Object;

class BottomAccel {
public:
    BottomAccel() = default;
    explicit BottomAccel(const Mesh& mesh,
                         vk::GeometryFlagBitsKHR geometryFlag = vk::GeometryFlagBitsKHR::eOpaque);

    uint64_t getBufferAddress() const { return buffer.getAddress(); }

private:
    vk::UniqueAccelerationStructureKHR accel;
    DeviceBuffer buffer;
};

class TopAccel {
public:
    TopAccel() = default;
    explicit TopAccel(const ArrayProxy<Object>& objects,
                      vk::GeometryFlagBitsKHR geometryFlag = vk::GeometryFlagBitsKHR::eOpaque);

    void rebuild(const ArrayProxy<Object>& objects);

    vk::AccelerationStructureKHR getAccel() const { return *accel; }
    vk::WriteDescriptorSetAccelerationStructureKHR getInfo() const { return {*accel}; }

private:
    vk::UniqueAccelerationStructureKHR accel;
    DeviceBuffer buffer;
    DeviceBuffer instanceBuffer;
    InstancesGeometry geometry;
};
