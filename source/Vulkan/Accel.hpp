#pragma once
#include "Geometry.hpp"

class Object;

class Accel
{
public:
    Accel() = default;

    Accel& operator=(Accel&& other) noexcept = default;

    vk::AccelerationStructureKHR GetAccel() const { return *accel; }
    uint64_t GetBufferAddress() const { return buffer.GetAddress(); }

protected:
    vk::UniqueAccelerationStructureKHR accel;
    DeviceBuffer buffer;
};

class BottomAccel : public Accel
{
public:
    BottomAccel() = default;
    BottomAccel(const Buffer& vertexBuffer, const Buffer& indexBuffer,
                size_t vertexCount, size_t primitiveCount, vk::GeometryFlagBitsKHR geomertyFlag);

    BottomAccel& operator=(BottomAccel&& other) noexcept = default;
};

class TopAccel : public Accel
{
public:
    TopAccel() = default;
    TopAccel(const std::vector<Object>& objects, vk::GeometryFlagBitsKHR geomertyFlag);
    TopAccel(const Object& object, vk::GeometryFlagBitsKHR geomertyFlag);
    void Rebuild(const std::vector<Object>& objects);

    TopAccel& operator=(TopAccel&& other) noexcept = default;

private:
    DeviceBuffer instanceBuffer;
    InstancesGeometry geometry;
};
