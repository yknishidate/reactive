#pragma once
#include "Geometry.hpp"

class Mesh;
class Object;

class BottomAccel
{
public:
    BottomAccel() = default;
    BottomAccel(const Mesh& mesh,
                vk::GeometryFlagBitsKHR geomertyFlag = vk::GeometryFlagBitsKHR::eOpaque);

    uint64_t GetBufferAddress() const { return buffer.GetAddress(); }

private:
    vk::UniqueAccelerationStructureKHR accel;
    DeviceBuffer buffer;
};

class TopAccel
{
public:
    TopAccel() = default;
    TopAccel(const std::vector<Object>& objects,
             vk::GeometryFlagBitsKHR geomertyFlaggeomertyFlag = vk::GeometryFlagBitsKHR::eOpaque);
    TopAccel(const Object& object,
             vk::GeometryFlagBitsKHR geomertyFlaggeomertyFlag = vk::GeometryFlagBitsKHR::eOpaque);

    void Rebuild(const std::vector<Object>& objects);

    vk::AccelerationStructureKHR GetAccel() const { return *accel; }

private:
    vk::UniqueAccelerationStructureKHR accel;
    DeviceBuffer buffer;
    DeviceBuffer instanceBuffer;
    InstancesGeometry geometry;
};
