#include "Geometry.hpp"

Geometry::Geometry(vk::GeometryFlagsKHR geometryFlag, size_t primitiveCount)
    : geometryFlag{ geometryFlag }
    , primitiveCount{ primitiveCount }
{
}

vk::DeviceSize Geometry::GetAccelSize() const
{
    constexpr auto buildType = vk::AccelerationStructureBuildTypeKHR::eDevice;
    const auto buildSizes = Graphics::GetDevice().getAccelerationStructureBuildSizesKHR(buildType, geometryInfo, primitiveCount);
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
                                     vk::GeometryFlagsKHR geometryFlag, size_t primitiveCount)
    : Geometry{ geometryFlag, primitiveCount }
{
    geometry = vk::AccelerationStructureGeometryKHR()
        .setGeometryType(vk::GeometryTypeKHR::eTriangles)
        .setGeometry({ triangles })
        .setFlags(geometryFlag);

    geometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
        .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
        .setGeometries(geometry);
}

InstancesGeometry::InstancesGeometry(vk::AccelerationStructureGeometryInstancesDataKHR instances,
                                     vk::GeometryFlagsKHR geometryFlag, size_t primitiveCount)
    : Geometry{ geometryFlag, primitiveCount }
{
    Update(instances);
}

void InstancesGeometry::Update(vk::AccelerationStructureGeometryInstancesDataKHR instances)
{
    geometry = vk::AccelerationStructureGeometryKHR()
        .setGeometryType(vk::GeometryTypeKHR::eInstances)
        .setGeometry({ instances })
        .setFlags(geometryFlag);

    geometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
        .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
        .setGeometries(geometry);
}
