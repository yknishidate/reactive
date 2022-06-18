#pragma once
#include "Buffer.hpp"

class Object;

class Accel
{
public:
    vk::AccelerationStructureKHR GetAccel() const { return *accel; }
    uint64_t GetBufferAddress() const { return buffer.GetAddress(); }

protected:
    vk::UniqueAccelerationStructureKHR accel;
    DeviceBuffer buffer;
};

class BottomAccel : public Accel
{
public:
    BottomAccel(const Buffer& vertexBuffer, const Buffer& indexBuffer,
                size_t vertexCount, size_t primitiveCount, vk::GeometryFlagBitsKHR geomertyFlag);
};

class TopAccel : public Accel
{
public:
    TopAccel(const std::vector<Object>& objects, vk::GeometryFlagBitsKHR geomertyFlag);
    TopAccel(const Object& object, vk::GeometryFlagBitsKHR geomertyFlag);
    void Rebuild(const std::vector<Object>& objects);

private:
    DeviceBuffer instanceBuffer;
    vk::GeometryFlagBitsKHR geomertyFlag;
};
