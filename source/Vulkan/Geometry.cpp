#include "Geometry.hpp"

Geometry::Geometry(vk::GeometryFlagsKHR geomertyFlag, size_t primitiveCount)
    : primitiveCount{ primitiveCount }
    , geomertyFlag{ geomertyFlag }
{
}

vk::DeviceSize Geometry::GetAccelSize() const
{
    const auto buildType = vk::AccelerationStructureBuildTypeKHR::eDevice;
    const auto buildSizes = Context::GetDevice().getAccelerationStructureBuildSizesKHR(buildType, geometryInfo, primitiveCount);
    return buildSizes.accelerationStructureSize;
}

vk::AccelerationStructureBuildGeometryInfoKHR Geometry::GetInfo() const
{
    return geometryInfo;
}

DeviceBuffer Geometry::CreateAccelBuffer() const
{
    return { vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
        vk::BufferUsageFlagBits::eShaderDeviceAddress, GetAccelSize() };
}

TrianglesGeometry::TrianglesGeometry(vk::AccelerationStructureGeometryTrianglesDataKHR triangles,
                                     vk::GeometryFlagsKHR geomertyFlag, size_t primitiveCount)
    : Geometry{ geomertyFlag, primitiveCount }
{
    geometry = vk::AccelerationStructureGeometryKHR()
        .setGeometryType(vk::GeometryTypeKHR::eTriangles)
        .setGeometry({ triangles })
        .setFlags(geomertyFlag);

    geometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
        .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
        .setGeometries(geometry);
}

InstancesGeometry::InstancesGeometry(vk::AccelerationStructureGeometryInstancesDataKHR instances,
                                     vk::GeometryFlagsKHR geomertyFlag, size_t primitiveCount)
    : Geometry{ geomertyFlag, primitiveCount }
{
    Update(instances, primitiveCount);
}

void InstancesGeometry::Update(vk::AccelerationStructureGeometryInstancesDataKHR instances, size_t primitiveCount)
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
