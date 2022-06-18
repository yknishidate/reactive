#pragma once
#include "Buffer.hpp"

class Geometry
{
public:
    vk::DeviceSize GetAccelSize() const;
    vk::AccelerationStructureBuildGeometryInfoKHR GetInfo() const;
    DeviceBuffer CreateAccelBuffer() const;

protected:
    vk::AccelerationStructureGeometryKHR geometry;
    vk::AccelerationStructureBuildGeometryInfoKHR geometryInfo;
    vk::GeometryFlagsKHR geomertyFlag;
    size_t primitiveCount;
};

class TrianglesGeometry : public Geometry
{
public:
    TrianglesGeometry() = default;
    TrianglesGeometry(vk::AccelerationStructureGeometryTrianglesDataKHR triangles,
                      vk::GeometryFlagsKHR geomertyFlag,
                      size_t primitiveCount)
    {
        this->primitiveCount = primitiveCount;
        this->geomertyFlag = geomertyFlag;
        geometry = vk::AccelerationStructureGeometryKHR()
            .setGeometryType(vk::GeometryTypeKHR::eTriangles)
            .setGeometry({ triangles })
            .setFlags(geomertyFlag);

        geometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
            .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
            .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
            .setGeometries(geometry);
    }

    TrianglesGeometry& operator=(TrianglesGeometry&&) = default;
};

class InstancesGeometry : public Geometry
{
public:
    InstancesGeometry() = default;
    InstancesGeometry(vk::AccelerationStructureGeometryInstancesDataKHR instances,
                      vk::GeometryFlagsKHR geomertyFlag,
                      size_t primitiveCount)
    {
        this->primitiveCount = primitiveCount;
        this->geomertyFlag = geomertyFlag;
        Update(instances, primitiveCount);
    }

    InstancesGeometry& operator=(InstancesGeometry&&) = default;

    void Update(vk::AccelerationStructureGeometryInstancesDataKHR instances, size_t primitiveCount)
    {
        geometry = vk::AccelerationStructureGeometryKHR()
            .setGeometryType(vk::GeometryTypeKHR::eInstances)
            .setGeometry({ instances })
            .setFlags(geomertyFlag);

        geometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
            .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
            .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
            .setGeometries(geometry);
    }
};
