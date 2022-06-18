#pragma once
#include "Buffer.hpp"

class Object;

class Accel
{
public:
    Accel() = default;
    vk::AccelerationStructureKHR GetAccel() const { return *accel; }
    uint64_t GetBufferAddress() const { return buffer.GetAddress(); }

    Accel& operator=(Accel&& other) noexcept
    {
        accel = std::move(other.accel);
        buffer = std::move(other.buffer);
        return *this;
    }

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

    BottomAccel& operator=(BottomAccel&& other) noexcept
    {
        accel = std::move(other.accel);
        buffer = std::move(other.buffer);
        return *this;
    }
};

class TopAccel : public Accel
{
public:
    TopAccel() = default;
    TopAccel(const std::vector<Object>& objects, vk::GeometryFlagBitsKHR geomertyFlag);
    TopAccel(const Object& object, vk::GeometryFlagBitsKHR geomertyFlag);
    void Rebuild(const std::vector<Object>& objects);

    TopAccel& operator=(TopAccel&& other) noexcept
    {
        accel = std::move(other.accel);
        buffer = std::move(other.buffer);
        instanceBuffer = std::move(other.instanceBuffer);
        geomertyFlag = std::move(other.geomertyFlag);
        return *this;
    }

private:
    DeviceBuffer instanceBuffer;
    vk::GeometryFlagBitsKHR geomertyFlag;
};
