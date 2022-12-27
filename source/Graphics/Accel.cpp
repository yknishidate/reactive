#include "Context.hpp"
#include "Accel.hpp"
#include "Scene/Object.hpp"

namespace
{
vk::UniqueAccelerationStructureKHR CreateAccel(vk::Buffer buffer, vk::DeviceSize size,
                                               vk::AccelerationStructureTypeKHR type)
{
    const auto accelInfo = vk::AccelerationStructureCreateInfoKHR()
        .setBuffer(buffer)
        .setSize(size)
        .setType(type);

    return Context::getDevice().createAccelerationStructureKHRUnique(accelInfo);
}

void BuildAccel(vk::AccelerationStructureKHR accel, vk::DeviceSize size, uint32_t primitiveCount,
                vk::AccelerationStructureBuildGeometryInfoKHR geometryInfo)
{
    const DeviceBuffer scratchBuffer{ BufferUsage::Scratch, size };

    geometryInfo.setScratchData(scratchBuffer.getAddress());
    geometryInfo.setDstAccelerationStructure(accel);

    Context::oneTimeSubmit(
        [&](vk::CommandBuffer commandBuffer)
    {
        vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{ primitiveCount, 0, 0, 0 };
    commandBuffer.buildAccelerationStructuresKHR(geometryInfo, &buildRangeInfo);
    });
}
}

BottomAccel::BottomAccel(const Mesh& mesh, vk::GeometryFlagBitsKHR geometryFlag)
{
    const auto triangleData = mesh.getTriangles();

    const size_t primitiveCount = mesh.getTriangleCount();
    const TrianglesGeometry geometry{ triangleData, geometryFlag, primitiveCount };
    const auto size = geometry.getAccelSize();
    const auto type = vk::AccelerationStructureTypeKHR::eBottomLevel;

    buffer = geometry.createAccelBuffer();
    accel = CreateAccel(buffer.getBuffer(), size, type);
    BuildAccel(*accel, size, primitiveCount, geometry.getInfo());
}

TopAccel::TopAccel(const ArrayProxy<Object>& objects, vk::GeometryFlagBitsKHR geometryFlag)
{
    const uint32_t primitiveCount = objects.size();

    std::vector<vk::AccelerationStructureInstanceKHR> instances;
    for (auto&& object : objects) {
        const auto instance = vk::AccelerationStructureInstanceKHR()
            .setTransform(object.transform.getVkMatrix())
            .setMask(0xFF)
            .setAccelerationStructureReference(object.getMesh().getAccel().getBufferAddress())
            .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);

        instances.push_back(instance);
    }

    instanceBuffer = DeviceBuffer{ BufferUsage::AccelInput, instances };

    const auto instancesData = vk::AccelerationStructureGeometryInstancesDataKHR()
        .setArrayOfPointers(false)
        .setData(instanceBuffer.getAddress());

    geometry = InstancesGeometry{ instancesData, geometryFlag, primitiveCount };
    const auto size = geometry.getAccelSize();

    buffer = geometry.createAccelBuffer();
    accel = CreateAccel(buffer.getBuffer(), size, vk::AccelerationStructureTypeKHR::eTopLevel);
    BuildAccel(*accel, size, primitiveCount, geometry.getInfo());
}

void TopAccel::rebuild(const std::vector<Object>& objects)
{
    const uint32_t primitiveCount = objects.size();

    std::vector<vk::AccelerationStructureInstanceKHR> instances;
    for (auto&& object : objects) {
        const auto instance = vk::AccelerationStructureInstanceKHR()
            .setTransform(object.transform.getVkMatrix())
            .setMask(0xFF)
            .setAccelerationStructureReference(object.getMesh().getAccel().getBufferAddress())
            .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);

        instances.push_back(instance);
    }

    instanceBuffer.copy(instances.data());

    const auto instancesData = vk::AccelerationStructureGeometryInstancesDataKHR()
        .setArrayOfPointers(false)
        .setData(instanceBuffer.getAddress());

    geometry.update(instancesData);
    const auto size = geometry.getAccelSize();
    BuildAccel(*accel, size, primitiveCount, geometry.getInfo());
}
