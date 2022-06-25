#include "Geometry.hpp"

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
