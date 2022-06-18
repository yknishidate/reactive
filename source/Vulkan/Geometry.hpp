#pragma once
#include "Buffer.hpp"

class Geometry
{
public:
    vk::DeviceSize GetAccelSize()
    {
        const auto buildType = vk::AccelerationStructureBuildTypeKHR::eDevice;
        const auto buildSizes = Vulkan::GetDevice().getAccelerationStructureBuildSizesKHR(buildType, geometryInfo, primitiveCount);
        return buildSizes.accelerationStructureSize;
    }

    vk::AccelerationStructureBuildGeometryInfoKHR GetInfo()
    {
        return geometryInfo;
    }

    DeviceBuffer CreateAccelBuffer()
    {
        return { vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
                 vk::BufferUsageFlagBits::eShaderDeviceAddress, GetAccelSize() };
    }

protected:
    vk::AccelerationStructureGeometryKHR geometry;
    vk::AccelerationStructureBuildGeometryInfoKHR geometryInfo;
    size_t primitiveCount;
    vk::GeometryFlagsKHR geomertyFlag;
};

class TrianglesGeometry : public Geometry
{
public:
    TrianglesGeometry() = default;
    TrianglesGeometry(vk::AccelerationStructureGeometryTrianglesDataKHR triangles,
                      vk::GeometryFlagsKHR geomertyFlag,
                      size_t primitiveCount)
    {
        geometry = vk::AccelerationStructureGeometryKHR()
            .setGeometryType(vk::GeometryTypeKHR::eTriangles)
            .setGeometry({ triangles })
            .setFlags(geomertyFlag);

        geometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
            .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
            .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
            .setGeometries(geometry);

        this->primitiveCount = primitiveCount;
        this->geomertyFlag = geomertyFlag;
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
