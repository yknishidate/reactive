#pragma once
#include "Buffer.hpp"

class Object;

class Accel
{
public:
    void InitAsBottom(const Buffer& vertexBuffer, const Buffer& indexBuffer,
                      size_t vertexCount, size_t primitiveCount);
    void InitAsTop(const std::vector<Object>& objects);
    void InitAsTop(const Object& object);
    void Rebuild(const std::vector<Object>& objects);

    vk::AccelerationStructureKHR GetAccel() const { return *accel; }

private:
    vk::UniqueAccelerationStructureKHR accel;
    Buffer buffer;
    Buffer instanceBuffer;
};

//class BottomAccel : public Accel
//{
//public:
//    void Init(const Buffer& vertexBuffer, const Buffer& indexBuffer,
//              size_t vertexCount, size_t primitiveCount);
//
//private:
//
//};