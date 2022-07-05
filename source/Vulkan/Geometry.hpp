#pragma once
#include "Buffer.hpp"

class Geometry
{
public:
    Geometry() = default;
    Geometry(vk::GeometryFlagsKHR geometryFlag, size_t primitiveCount);

    vk::DeviceSize GetAccelSize() const;
    vk::AccelerationStructureBuildGeometryInfoKHR GetInfo() const;
    DeviceBuffer CreateAccelBuffer() const;

protected:
    vk::AccelerationStructureGeometryKHR geometry;
    vk::AccelerationStructureBuildGeometryInfoKHR geometryInfo;
    vk::GeometryFlagsKHR geometryFlag;
    size_t primitiveCount{};
};

class TrianglesGeometry : public Geometry
{
public:
    TrianglesGeometry() = default;
    TrianglesGeometry(vk::AccelerationStructureGeometryTrianglesDataKHR triangles,
                      vk::GeometryFlagsKHR geometryFlag, size_t primitiveCount);
};

class InstancesGeometry : public Geometry
{
public:
    InstancesGeometry() = default;
    InstancesGeometry(vk::AccelerationStructureGeometryInstancesDataKHR instances,
                      vk::GeometryFlagsKHR geometryFlag, size_t primitiveCount);

    void Update(vk::AccelerationStructureGeometryInstancesDataKHR instances);
};
