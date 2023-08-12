#pragma once
#include "Buffer.hpp"

namespace rv {
class Geometry {
public:
    Geometry() = default;
    Geometry(vk::GeometryFlagsKHR geometryFlag, size_t primitiveCount);

    vk::DeviceSize getAccelSize() const;
    vk::AccelerationStructureBuildGeometryInfoKHR getInfo() const;
    Buffer createAccelBuffer() const;

protected:
    vk::AccelerationStructureGeometryKHR geometry;
    vk::AccelerationStructureBuildGeometryInfoKHR geometryInfo;
    vk::GeometryFlagsKHR geometryFlag;
    size_t primitiveCount{};
};

class TrianglesGeometry : public Geometry {
public:
    TrianglesGeometry() = default;
    TrianglesGeometry(vk::AccelerationStructureGeometryTrianglesDataKHR triangles,
                      vk::GeometryFlagsKHR geometryFlag,
                      size_t primitiveCount);
};

class InstancesGeometry : public Geometry {
public:
    InstancesGeometry() = default;
    InstancesGeometry(vk::AccelerationStructureGeometryInstancesDataKHR instances,
                      vk::GeometryFlagsKHR geometryFlag,
                      size_t primitiveCount);

    void update(vk::AccelerationStructureGeometryInstancesDataKHR instances);
};
}  // namespace rv
