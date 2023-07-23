#include "Accel.hpp"

BottomAccel::BottomAccel(const Context* context, BottomAccelCreateInfo createInfo)
    : context{context} {
    vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData;
    trianglesData.setVertexFormat(vk::Format::eR32G32B32Sfloat);
    trianglesData.setVertexData(createInfo.vertexBuffer.getAddress());
    trianglesData.setVertexStride(createInfo.vertexStride);
    trianglesData.setMaxVertex(createInfo.vertexCount);
    trianglesData.setIndexType(vk::IndexType::eUint32);
    trianglesData.setIndexData(createInfo.indexBuffer.getAddress());

    vk::AccelerationStructureGeometryDataKHR geometryData;
    geometryData.setTriangles(trianglesData);

    vk::AccelerationStructureGeometryKHR geometry;
    geometry.setGeometryType(vk::GeometryTypeKHR::eTriangles);
    geometry.setGeometry({geometryData});
    geometry.setFlags(createInfo.geometryFlags);

    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
    buildGeometryInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
    buildGeometryInfo.setFlags(createInfo.buildFlags);
    buildGeometryInfo.setGeometries(geometry);

    size_t primitiveCount = createInfo.triangleCount;
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

TopAccel::TopAccel(const Context* context, TopAccelCreateInfo createInfo)
    : context{context},
      geometryFlags{createInfo.geometryFlags},
      buildFlags{createInfo.buildFlags},
      buildType{createInfo.buildType} {
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
    geometry.setFlags(geometryFlags);

    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
    buildGeometryInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
    buildGeometryInfo.setFlags(buildFlags);
    buildGeometryInfo.setGeometries(geometry);

    const uint32_t primitiveCount = instances.size();
    auto buildSizesInfo = context->getDevice().getAccelerationStructureBuildSizesKHR(
        buildType, buildGeometryInfo, primitiveCount);

    buffer = context->createDeviceBuffer({
        .usage = BufferUsage::AccelStorage,
        .size = buildSizesInfo.accelerationStructureSize,
    });

    spdlog::info("buildSizesInfo.accelerationStructureSize: {}",
                 buildSizesInfo.accelerationStructureSize);

    accel = context->getDevice().createAccelerationStructureKHRUnique(
        vk::AccelerationStructureCreateInfoKHR{}
            .setBuffer(buffer.getBuffer())
            .setSize(buildSizesInfo.accelerationStructureSize)
            .setType(vk::AccelerationStructureTypeKHR::eTopLevel));

    scratchBuffer = context->createHostBuffer({
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

void TopAccel::update(vk::CommandBuffer commandBuffer,
                      ArrayProxy<std::pair<const BottomAccel*, glm::mat4>> bottomAccels) {
    std::vector<vk::AccelerationStructureInstanceKHR> instances;
    for (auto& [bottomAccel, transform] : bottomAccels) {
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

    instanceBuffer.copy(instances.data());

    vk::AccelerationStructureGeometryInstancesDataKHR instancesData;
    instancesData.setArrayOfPointers(false);
    instancesData.setData(instanceBuffer.getAddress());

    vk::AccelerationStructureGeometryKHR geometry;
    geometry.setGeometryType(vk::GeometryTypeKHR::eInstances);
    geometry.setGeometry({instancesData});
    geometry.setFlags(geometryFlags);

    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
    buildGeometryInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
    buildGeometryInfo.setFlags(buildFlags);
    buildGeometryInfo.setGeometries(geometry);

    buildGeometryInfo.setMode(vk::BuildAccelerationStructureModeKHR::eUpdate);  // Not build
    // buildGeometryInfo.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);  // Not build
    buildGeometryInfo.setDstAccelerationStructure(*accel);
    buildGeometryInfo.setScratchData(scratchBuffer.getAddress());

    vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
    buildRangeInfo.setPrimitiveCount(instances.size());
    buildRangeInfo.setPrimitiveOffset(0);
    buildRangeInfo.setFirstVertex(0);
    buildRangeInfo.setTransformOffset(0);
    commandBuffer.buildAccelerationStructuresKHR(buildGeometryInfo, &buildRangeInfo);

    // Create a memory barrier for the acceleration structure
    vk::MemoryBarrier2 memoryBarrier{};
    memoryBarrier.setSrcStageMask(vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR);
    memoryBarrier.setDstStageMask(vk::PipelineStageFlagBits2::eRayTracingShaderKHR);
    memoryBarrier.setSrcAccessMask(vk::AccessFlagBits2::eAccelerationStructureWriteKHR);
    memoryBarrier.setDstAccessMask(vk::AccessFlagBits2::eAccelerationStructureReadKHR);

    // Pipeline barrier to ensure that the build has finished before the ray tracing shader starts
    vk::DependencyInfoKHR dependencyInfo{};
    dependencyInfo.setMemoryBarrierCount(1);
    dependencyInfo.setPMemoryBarriers(&memoryBarrier);
    commandBuffer.pipelineBarrier2(dependencyInfo);
}
