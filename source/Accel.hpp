#pragma once
#include "Buffer.hpp"

class Object;

class Accel
{
public:
    void InitAsBottom(const Buffer& vertexBuffer, const Buffer& indexBuffer,
                      size_t vertexStride, size_t vertexCount, size_t primitiveCount);

    void InitAsTop(const Accel& bottom);
    void InitAsTop(const Accel& bottom, const vk::TransformMatrixKHR& transform);
    void InitAsTop(const Object& object);
    void InitAsTop(const std::vector<Object>& objects);

    vk::AccelerationStructureKHR GetAccel() const { return *accel; }

private:
    vk::UniqueAccelerationStructureKHR accel;
    Buffer buffer;
};
