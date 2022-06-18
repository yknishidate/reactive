#pragma once
#include "Buffer.hpp"

class Geometry
{
public:
    Geometry() = default;

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
};

class TrianglesGeometry : public Geometry
{
public:
    TrianglesGeometry(vk::AccelerationStructureGeometryTrianglesDataKHR triangles,
                      vk::GeometryFlagsKHR geomertyFlag,
                      size_t primitiveCount)
    {
        const auto type = vk::AccelerationStructureTypeKHR::eBottomLevel;

        geometry = vk::AccelerationStructureGeometryKHR()
            .setGeometryType(vk::GeometryTypeKHR::eTriangles)
            .setGeometry({ triangles })
            .setFlags(geomertyFlag);

        geometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
            .setType(type)
            .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
            .setGeometries(geometry);

        this->primitiveCount = primitiveCount;
    }
};

class InstancesGeometry : public Geometry
{
public:
    InstancesGeometry(vk::AccelerationStructureGeometryInstancesDataKHR instances,
                      vk::GeometryFlagsKHR geomertyFlag,
                      size_t primitiveCount)
    {
        const auto type = vk::AccelerationStructureTypeKHR::eTopLevel;

        geometry = vk::AccelerationStructureGeometryKHR()
            .setGeometryType(vk::GeometryTypeKHR::eInstances)
            .setGeometry({ instances })
            .setFlags(geomertyFlag);

        geometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
            .setType(type)
            .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
            .setGeometries(geometry);

        this->primitiveCount = primitiveCount;
    }
};
