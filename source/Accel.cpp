#include "Vulkan.hpp"
#include "Accel.hpp"

namespace
{
    vk::UniqueAccelerationStructureKHR CreateAccel(vk::Buffer buffer, vk::DeviceSize size,
                                                   vk::AccelerationStructureTypeKHR type)
    {
        vk::AccelerationStructureCreateInfoKHR createInfo;
        createInfo.setBuffer(buffer);
        createInfo.setSize(size);
        createInfo.setType(type);
        return Vulkan::Device.createAccelerationStructureKHRUnique(createInfo);
    }

    vk::DeviceSize GetAccelSize(vk::AccelerationStructureBuildGeometryInfoKHR geometryInfo, size_t primitiveCount)
    {
        auto buildSizes = Vulkan::Device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, geometryInfo, primitiveCount);
        return buildSizes.accelerationStructureSize;
    }

    Buffer CreateAccelBuffer(vk::DeviceSize size,
                             vk::AccelerationStructureTypeKHR type,
                             vk::AccelerationStructureGeometryKHR geometry)
    {
        Buffer buffer;
        buffer.InitOnDevice(size,
                            vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
                            vk::BufferUsageFlagBits::eShaderDeviceAddress);
        return buffer;
    }

    void BuildAccel(vk::AccelerationStructureKHR accel, vk::DeviceSize size, uint32_t primitiveCount,
                    vk::AccelerationStructureBuildGeometryInfoKHR geometryInfo)
    {
        Buffer scratchBuffer;
        scratchBuffer.InitOnDevice(size, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress);

        geometryInfo.setScratchData(scratchBuffer.GetAddress());
        geometryInfo.setDstAccelerationStructure(accel);

        Vulkan::OneTimeSubmit(
            [&](vk::CommandBuffer commandBuffer)
            {
                vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{ primitiveCount, 0, 0, 0 };
                commandBuffer.buildAccelerationStructuresKHR(geometryInfo, &buildRangeInfo);
            });
    }
}

void Accel::InitAsBottom(const Buffer& vertexBuffer, const Buffer& indexBuffer, size_t vertexStride, size_t vertexCount, size_t primitiveCount)
{
    vk::AccelerationStructureGeometryTrianglesDataKHR triangleData;
    triangleData.setVertexFormat(vk::Format::eR32G32B32Sfloat);
    triangleData.setVertexData(vertexBuffer.GetAddress());
    triangleData.setVertexStride(vertexStride);
    triangleData.setMaxVertex(vertexCount);
    triangleData.setIndexType(vk::IndexType::eUint32);
    triangleData.setIndexData(indexBuffer.GetAddress());

    vk::AccelerationStructureGeometryKHR geometry;
    geometry.setGeometryType(vk::GeometryTypeKHR::eTriangles);
    geometry.setGeometry({ triangleData });
    geometry.setFlags(vk::GeometryFlagBitsKHR::eOpaque);

    vk::AccelerationStructureTypeKHR type = vk::AccelerationStructureTypeKHR::eBottomLevel;

    vk::AccelerationStructureBuildGeometryInfoKHR geometryInfo;
    geometryInfo.setType(type);
    geometryInfo.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
    geometryInfo.setGeometries(geometry);

    vk::DeviceSize size = GetAccelSize(geometryInfo, primitiveCount);
    buffer = CreateAccelBuffer(size, type, geometry);
    accel = CreateAccel(buffer.GetBuffer(), size, type);
    BuildAccel(*accel, size, primitiveCount, geometryInfo);
}

void Accel::InitAsTop(const Accel& bottom)
{
    vk::TransformMatrixKHR transformMatrix = std::array{
        std::array{1.0f, 0.0f, 0.0f, 0.0f},
        std::array{0.0f, 1.0f, 0.0f, 0.0f},
        std::array{0.0f, 0.0f, 1.0f, 0.0f} };

    vk::AccelerationStructureInstanceKHR instance;
    instance.setTransform(transformMatrix);
    instance.setMask(0xFF);
    instance.setAccelerationStructureReference(bottom.buffer.GetAddress());
    instance.setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);

    Buffer instanceBuffer;
    instanceBuffer.InitOnHost(sizeof(vk::AccelerationStructureInstanceKHR),
                              vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                              vk::BufferUsageFlagBits::eShaderDeviceAddress);
    instanceBuffer.Copy(&instance);

    vk::AccelerationStructureGeometryInstancesDataKHR instancesData;
    instancesData.setArrayOfPointers(false);
    instancesData.setData(instanceBuffer.GetAddress());

    vk::AccelerationStructureGeometryKHR geometry;
    geometry.setGeometryType(vk::GeometryTypeKHR::eInstances);
    geometry.setGeometry({ instancesData });
    geometry.setFlags(vk::GeometryFlagBitsKHR::eOpaque);

    vk::AccelerationStructureTypeKHR type = vk::AccelerationStructureTypeKHR::eTopLevel;
    uint32_t primitiveCount = 1;

    vk::AccelerationStructureBuildGeometryInfoKHR geometryInfo;
    geometryInfo.setType(type);
    geometryInfo.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
    geometryInfo.setGeometries(geometry);

    vk::DeviceSize size = GetAccelSize(geometryInfo, primitiveCount);
    buffer = CreateAccelBuffer(size, type, geometry);
    accel = CreateAccel(buffer.GetBuffer(), size, type);
    BuildAccel(*accel, size, primitiveCount, geometryInfo);
}
