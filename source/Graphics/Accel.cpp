#include "Accel.hpp"
#include "Scene/Mesh.hpp"

// namespace {
// vk::UniqueAccelerationStructureKHR createAccel(vk::Buffer buffer,
//                                                vk::DeviceSize size,
//                                                vk::AccelerationStructureTypeKHR type) {
//     vk::AccelerationStructureCreateInfoKHR accelInfo;
//     accelInfo.setBuffer(buffer);
//     accelInfo.setSize(size);
//     accelInfo.setType(type);
//     return Context::getDevice().createAccelerationStructureKHRUnique(accelInfo);
// }
//
// void buildAccel(vk::AccelerationStructureKHR accel,
//                 vk::DeviceSize size,
//                 uint32_t primitiveCount,
//                 vk::AccelerationStructureBuildGeometryInfoKHR geometryInfo) {
//     DeviceBuffer scratchBuffer{BufferUsage::Scratch, size};
//
//     geometryInfo.setScratchData(scratchBuffer.getAddress());
//     geometryInfo.setDstAccelerationStructure(accel);
//
//     Context::oneTimeSubmit([&](vk::CommandBuffer commandBuffer) {
//         vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{primitiveCount, 0, 0, 0};
//         commandBuffer.buildAccelerationStructuresKHR(geometryInfo, &buildRangeInfo);
//     });
// }
// }  // namespace

BottomAccel::BottomAccel(const Context* context, BottomAccelCreateInfo createInfo)
    : context{context} {
    const Mesh* mesh = createInfo.mesh;

    vk::AccelerationStructureGeometryDataKHR geometryData;
    geometryData.setTriangles(mesh->getTrianglesData());

    vk::AccelerationStructureGeometryKHR geometry;
    geometry.setGeometryType(vk::GeometryTypeKHR::eTriangles);
    geometry.setGeometry({geometryData});
    geometry.setFlags(createInfo.geometryFlags);

    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
    buildGeometryInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
    buildGeometryInfo.setFlags(createInfo.buildFlags);
    buildGeometryInfo.setGeometries(geometry);

    size_t primitiveCount = mesh->getTriangleCount();
    auto buildSizesInfo = context->getDevice().getAccelerationStructureBuildSizesKHR(
        createInfo.buildType, buildGeometryInfo, primitiveCount);

    buffer = context->createDeviceBuffer({
        .usage = BufferUsage::AccelStorage,
        .size = buildSizesInfo.accelerationStructureSize,
    });

    accel = context->getDevice().createAccelerationStructureKHRUnique(
        vk::AccelerationStructureCreateInfoKHR{}
            .setBuffer(buffer.getBuffer())
            .setSize(buildSizesInfo.accelerationStructureSize)
            .setType(vk::AccelerationStructureTypeKHR::eBottomLevel));

    Buffer scratchBuffer = context->createHostBuffer({
        .usage = BufferUsage::Scratch,
        .size = buildSizesInfo.buildScratchSize,
    });

    buildGeometryInfo.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);
    buildGeometryInfo.setDstAccelerationStructure(*accel);
    buildGeometryInfo.setScratchData(scratchBuffer.getAddress());

    context->oneTimeSubmit([&](vk::CommandBuffer commandBuffer) {
        vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
        buildRangeInfo.setPrimitiveCount(primitiveCount);
        buildRangeInfo.setPrimitiveOffset(0);
        buildRangeInfo.setFirstVertex(0);
        buildRangeInfo.setTransformOffset(0);
        commandBuffer.buildAccelerationStructuresKHR(buildGeometryInfo, &buildRangeInfo);
    });
}

TopAccel::TopAccel(const Context* context, TopAccelCreateInfo createInfo) {
    std::vector<vk::AccelerationStructureInstanceKHR> instances;
    for (auto& [bottomAccel, transform] : createInfo.bottomAccels) {
        // Convert glm::mat4 to vk::TransformMatrixKHR
        const glm::mat4 transposedMatrix = glm::transpose(transform);
        // TODO: direct to vk::Transform
        std::array<std::array<float, 4>, 3> data;
        std::memcpy(&data, &transposedMatrix, sizeof(vk::TransformMatrixKHR));

        instances.push_back(
            vk::AccelerationStructureInstanceKHR()
                .setTransform({data})
                .setInstanceCustomIndex(0)
                .setMask(0xFF)
                .setInstanceShaderBindingTableRecordOffset(0)
                .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable)
                .setAccelerationStructureReference(bottomAccel->getBufferAddress()));
    }

    instanceBuffer = context->createDeviceBuffer({
        .usage = BufferUsage::AccelInput,
        .size = sizeof(vk::AccelerationStructureInstanceKHR) * instances.size(),
        .data = instances.data(),
    });

    vk::AccelerationStructureGeometryInstancesDataKHR instancesData;
    instancesData.setArrayOfPointers(false);
    instancesData.setData(instanceBuffer.getAddress());

    vk::AccelerationStructureGeometryKHR geometry;
    geometry.setGeometryType(vk::GeometryTypeKHR::eInstances);
    geometry.setGeometry({instancesData});
    geometry.setFlags(createInfo.geometryFlags);

    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
    buildGeometryInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
    buildGeometryInfo.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
    buildGeometryInfo.setGeometries(geometry);

    const uint32_t primitiveCount = instances.size();
    auto buildSizesInfo = context->getDevice().getAccelerationStructureBuildSizesKHR(
        createInfo.buildType, buildGeometryInfo, primitiveCount);

    buffer = context->createDeviceBuffer({
        .usage = BufferUsage::AccelStorage,
        .size = buildSizesInfo.accelerationStructureSize,
    });

    accel = context->getDevice().createAccelerationStructureKHRUnique(
        vk::AccelerationStructureCreateInfoKHR{}
            .setBuffer(buffer.getBuffer())
            .setSize(buildSizesInfo.accelerationStructureSize)
            .setType(vk::AccelerationStructureTypeKHR::eTopLevel));

    Buffer scratchBuffer = context->createHostBuffer({
        .usage = BufferUsage::Scratch,
        .size = buildSizesInfo.buildScratchSize,
    });

    buildGeometryInfo.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);
    buildGeometryInfo.setDstAccelerationStructure(*accel);
    buildGeometryInfo.setScratchData(scratchBuffer.getAddress());

    context->oneTimeSubmit([&](vk::CommandBuffer commandBuffer) {
        vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
        buildRangeInfo.setPrimitiveCount(primitiveCount);
        buildRangeInfo.setPrimitiveOffset(0);
        buildRangeInfo.setFirstVertex(0);
        buildRangeInfo.setTransformOffset(0);
        commandBuffer.buildAccelerationStructuresKHR(buildGeometryInfo, &buildRangeInfo);
    });
}

// TopAccel::TopAccel(const ArrayProxy<Object>& objects, vk::GeometryFlagBitsKHR geometryFlag) {
//     // Gather instances
//     std::vector<vk::AccelerationStructureInstanceKHR> instances;
//     for (auto& object : objects) {
//         instances.push_back(object.createInstance());
//     }
//
//     // Create instance buffer
//     instanceBuffer = DeviceBuffer{BufferUsage::AccelInput, instances};
//
//     vk::AccelerationStructureGeometryInstancesDataKHR instancesData;
//     instancesData.setArrayOfPointers(false);
//     instancesData.setData(instanceBuffer.getAddress());
//
//     uint32_t primitiveCount = objects.size();
//     geometry = InstancesGeometry{instancesData, geometryFlag, primitiveCount};
//     auto size = geometry.getAccelSize();
//
//     buffer = geometry.createAccelBuffer();
//     accel = createAccel(buffer.getBuffer(), size, vk::AccelerationStructureTypeKHR::eTopLevel);
//     buildAccel(*accel, size, primitiveCount, geometry.getInfo());
// }
//
// void TopAccel::rebuild(const ArrayProxy<Object>& objects) {
//     // Gather instances
//     std::vector<vk::AccelerationStructureInstanceKHR> instances;
//     for (auto& object : objects) {
//         instances.push_back(object.createInstance());
//     }
//
//     // Copy to buffer
//     instanceBuffer.copy(instances.data());
//
//     vk::AccelerationStructureGeometryInstancesDataKHR instancesData;
//     instancesData.setArrayOfPointers(false);
//     instancesData.setData(instanceBuffer.getAddress());
//
//     uint32_t primitiveCount = objects.size();
//     geometry.update(instancesData);
//     auto size = geometry.getAccelSize();
//     buildAccel(*accel, size, primitiveCount, geometry.getInfo());
// }