#include "Vulkan.hpp"
#include "Accel.hpp"
#include "Object.hpp"

namespace
{
    vk::UniqueAccelerationStructureKHR CreateAccel(vk::Buffer buffer, vk::DeviceSize size,
                                                   vk::AccelerationStructureTypeKHR type)
    {
        const auto accelInfo = vk::AccelerationStructureCreateInfoKHR()
            .setBuffer(buffer)
            .setSize(size)
            .setType(type);

        return Vulkan::GetDevice().createAccelerationStructureKHRUnique(accelInfo);
    }

    vk::DeviceSize GetAccelSize(vk::AccelerationStructureBuildGeometryInfoKHR geometryInfo, size_t primitiveCount)
    {
        auto buildSizes = Vulkan::GetDevice().getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, geometryInfo, primitiveCount);
        return buildSizes.accelerationStructureSize;
    }

    DeviceBuffer CreateAccelBuffer(vk::DeviceSize size,
                                   vk::AccelerationStructureTypeKHR type,
                                   vk::AccelerationStructureGeometryKHR geometry)
    {
        return DeviceBuffer{ vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
                              vk::BufferUsageFlagBits::eShaderDeviceAddress, size };
    }

    void BuildAccel(vk::AccelerationStructureKHR accel, vk::DeviceSize size, uint32_t primitiveCount,
                    vk::AccelerationStructureBuildGeometryInfoKHR geometryInfo)
    {
        DeviceBuffer scratchBuffer{ vk::BufferUsageFlagBits::eStorageBuffer |
                                    vk::BufferUsageFlagBits::eShaderDeviceAddress, size };

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

BottomAccel::BottomAccel(const Buffer& vertexBuffer, const Buffer& indexBuffer,
                         size_t vertexCount, size_t primitiveCount,
                         vk::GeometryFlagBitsKHR geomertyFlag)
{
    const auto triangleData = vk::AccelerationStructureGeometryTrianglesDataKHR()
        .setVertexFormat(vk::Format::eR32G32B32Sfloat)
        .setVertexData(vertexBuffer.GetAddress())
        .setVertexStride(sizeof(Vertex))
        .setMaxVertex(vertexCount)
        .setIndexType(vk::IndexType::eUint32)
        .setIndexData(indexBuffer.GetAddress());

    const auto geometry = vk::AccelerationStructureGeometryKHR()
        .setGeometryType(vk::GeometryTypeKHR::eTriangles)
        .setGeometry({ triangleData })
        .setFlags(geomertyFlag);

    const auto type = vk::AccelerationStructureTypeKHR::eBottomLevel;
    const auto geometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
        .setType(type)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
        .setGeometries(geometry);

    const vk::DeviceSize size = GetAccelSize(geometryInfo, primitiveCount);
    buffer = CreateAccelBuffer(size, type, geometry);
    accel = CreateAccel(buffer.GetBuffer(), size, type);
    BuildAccel(*accel, size, primitiveCount, geometryInfo);
}

TopAccel::TopAccel(const std::vector<Object>& objects, vk::GeometryFlagBitsKHR geomertyFlag)
{
    this->geomertyFlag = geomertyFlag;
    const uint32_t primitiveCount = objects.size();

    std::vector<vk::AccelerationStructureInstanceKHR> instances;
    for (auto&& object : objects) {
        const auto instance = vk::AccelerationStructureInstanceKHR()
            .setTransform(object.transform.GetVkMatrix())
            .setMask(0xFF)
            .setAccelerationStructureReference(object.GetMesh().GetAccel().GetBufferAddress())
            .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);

        instances.push_back(instance);
    }

    instanceBuffer = DeviceBuffer{ vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                                   vk::BufferUsageFlagBits::eShaderDeviceAddress |
                                   vk::BufferUsageFlagBits::eTransferDst,
                                   instances };

    const auto instancesData = vk::AccelerationStructureGeometryInstancesDataKHR()
        .setArrayOfPointers(false)
        .setData(instanceBuffer.GetAddress());

    const auto geometry = vk::AccelerationStructureGeometryKHR()
        .setGeometryType(vk::GeometryTypeKHR::eInstances)
        .setGeometry({ instancesData })
        .setFlags(geomertyFlag);

    const auto type = vk::AccelerationStructureTypeKHR::eTopLevel;
    const  auto geometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
        .setType(type)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
        .setGeometries(geometry);

    const vk::DeviceSize size = GetAccelSize(geometryInfo, primitiveCount);
    buffer = CreateAccelBuffer(size, type, geometry);
    accel = CreateAccel(buffer.GetBuffer(), size, type);
    BuildAccel(*accel, size, primitiveCount, geometryInfo);
}

TopAccel::TopAccel(const Object& object, vk::GeometryFlagBitsKHR geomertyFlag)
    : TopAccel(std::vector{ object }, geomertyFlag)
{
}

void TopAccel::Rebuild(const std::vector<Object>& objects)
{
    const uint32_t primitiveCount = objects.size();

    std::vector<vk::AccelerationStructureInstanceKHR> instances;
    for (auto&& object : objects) {
        const auto instance = vk::AccelerationStructureInstanceKHR()
            .setTransform(object.transform.GetVkMatrix())
            .setMask(0xFF)
            .setAccelerationStructureReference(object.GetMesh().GetAccel().GetBufferAddress())
            .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);

        instances.push_back(instance);
    }

    instanceBuffer.Copy(instances.data());

    const auto instancesData = vk::AccelerationStructureGeometryInstancesDataKHR()
        .setArrayOfPointers(false)
        .setData(instanceBuffer.GetAddress());

    const auto geometry = vk::AccelerationStructureGeometryKHR()
        .setGeometryType(vk::GeometryTypeKHR::eInstances)
        .setGeometry({ instancesData })
        .setFlags(geomertyFlag);

    const auto type = vk::AccelerationStructureTypeKHR::eTopLevel;
    const auto geometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
        .setType(type)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
        .setGeometries(geometry);

    const vk::DeviceSize size = GetAccelSize(geometryInfo, primitiveCount);
    BuildAccel(*accel, size, primitiveCount, geometryInfo);
}
