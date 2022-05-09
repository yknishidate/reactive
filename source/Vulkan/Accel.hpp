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
    void Init(const Buffer& vertexBuffer, const Buffer& indexBuffer,
              size_t vertexCount, size_t primitiveCount, vk::GeometryFlagBitsKHR geomertyFlag);

private:

};

class TopAccel : public Accel
{
public:
    void Init(const std::vector<Object>& objects, vk::GeometryFlagBitsKHR geomertyFlag);
    void Init(const Object& object, vk::GeometryFlagBitsKHR geomertyFlag);
    void Rebuild(const std::vector<Object>& objects);

private:
    DeviceBuffer instanceBuffer;
    vk::GeometryFlagBitsKHR geomertyFlag;
};
