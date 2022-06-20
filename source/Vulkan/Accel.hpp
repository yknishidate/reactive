#pragma once
#include "Geometry.hpp"

class Mesh;
class Object;

class BottomAccel
{
public:
    BottomAccel() = default;
    BottomAccel(const Mesh& mesh, vk::GeometryFlagBitsKHR geomertyFlag);

    BottomAccel& operator=(BottomAccel&& other) noexcept = default;

    vk::AccelerationStructureKHR GetAccel() const { return *accel; }
    uint64_t GetBufferAddress() const { return buffer.GetAddress(); }

private:
    vk::UniqueAccelerationStructureKHR accel;
    DeviceBuffer buffer;
};

class TopAccel
{
public:
    TopAccel() = default;
    TopAccel(const std::vector<Object>& objects, vk::GeometryFlagBitsKHR geomertyFlag);
    TopAccel(const Object& object, vk::GeometryFlagBitsKHR geomertyFlag);
    void Rebuild(const std::vector<Object>& objects);

    TopAccel& operator=(TopAccel&& other) noexcept = default;

    vk::AccelerationStructureKHR GetAccel() const { return *accel; }
    uint64_t GetBufferAddress() const { return buffer.GetAddress(); }

private:
    vk::UniqueAccelerationStructureKHR accel;
    DeviceBuffer buffer;
    DeviceBuffer instanceBuffer;
    InstancesGeometry geometry;
};
