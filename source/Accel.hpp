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
    Buffer buffer;
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
    void Init(const std::vector<Object>& objects, vk::GeometryFlagBitsKHR geomertyFlag = vk::GeometryFlagBitsKHR::eOpaque);
    void Init(const Object& object, vk::GeometryFlagBitsKHR geomertyFlag = vk::GeometryFlagBitsKHR::eOpaque);
    void Rebuild(const std::vector<Object>& objects);

private:
    Buffer instanceBuffer;
    vk::GeometryFlagBitsKHR geomertyFlag;
};
