#include "Graphics/Accel.hpp"

namespace {
vk::TransformMatrixKHR toVkMatrix(const glm::mat4& matrix) {
    const glm::mat4 transposedMatrix = glm::transpose(matrix);
    vk::TransformMatrixKHR vkMatrix;
    std::memcpy(&vkMatrix, &transposedMatrix, sizeof(vk::TransformMatrixKHR));
    return vkMatrix;
}
}  // namespace

namespace rv {
BottomAccel::BottomAccel(const Context* context, BottomAccelCreateInfo createInfo)
    : context{context} {
    vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData;
    trianglesData.setVertexFormat(vk::Format::eR32G32B32Sfloat);
    trianglesData.setVertexData(createInfo.vertexBuffer->getAddress());
    trianglesData.setVertexStride(createInfo.vertexStride);
    trianglesData.setMaxVertex(createInfo.vertexCount);
    trianglesData.setIndexType(vk::IndexTypeValue<uint32_t>::value);
    trianglesData.setIndexData(createInfo.indexBuffer->getAddress());

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

    buffer = context->createBuffer({
        .usage = BufferUsage::AccelStorage,
        .memory = MemoryUsage::Device,
        .size = buildSizesInfo.accelerationStructureSize,
    });

    accel = context->getDevice().createAccelerationStructureKHRUnique(
        vk::AccelerationStructureCreateInfoKHR{}
            .setBuffer(buffer->getBuffer())
            .setSize(buildSizesInfo.accelerationStructureSize)
            .setType(vk::AccelerationStructureTypeKHR::eBottomLevel));

    BufferHandle scratchBuffer = context->createBuffer({
        .usage = BufferUsage::Scratch,
        .memory = MemoryUsage::Host,
        .size = buildSizesInfo.buildScratchSize,
    });

    buildGeometryInfo.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);
    buildGeometryInfo.setDstAccelerationStructure(*accel);
    buildGeometryInfo.setScratchData(scratchBuffer->getAddress());

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
    for (auto& instance : createInfo.accelInstances) {
        instances.push_back(
            vk::AccelerationStructureInstanceKHR()
                .setTransform(toVkMatrix(instance.transform))
                .setInstanceCustomIndex(0)
                .setMask(0xFF)
                .setInstanceShaderBindingTableRecordOffset(instance.sbtOffset)
                .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable)
                .setAccelerationStructureReference(instance.bottomAccel->getBufferAddress()));
    }

    instanceBuffer = context->createBuffer({
        .usage = BufferUsage::AccelInput,
        .memory = MemoryUsage::DeviceHost,
        .size = sizeof(vk::AccelerationStructureInstanceKHR) * instances.size(),
        .data = instances.data(),
    });

    vk::AccelerationStructureGeometryInstancesDataKHR instancesData;
    instancesData.setArrayOfPointers(false);
    instancesData.setData(instanceBuffer->getAddress());

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

    buffer = context->createBuffer({
        .usage = BufferUsage::AccelStorage,
        .memory = MemoryUsage::Device,
        .size = buildSizesInfo.accelerationStructureSize,
    });

    accel = context->getDevice().createAccelerationStructureKHRUnique(
        vk::AccelerationStructureCreateInfoKHR{}
            .setBuffer(buffer->getBuffer())
            .setSize(buildSizesInfo.accelerationStructureSize)
            .setType(vk::AccelerationStructureTypeKHR::eTopLevel));

    scratchBuffer = context->createBuffer({
        .usage = BufferUsage::Scratch,
        .memory = MemoryUsage::Device,
        .size = buildSizesInfo.buildScratchSize,
    });

    buildGeometryInfo.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);
    buildGeometryInfo.setDstAccelerationStructure(*accel);
    buildGeometryInfo.setScratchData(scratchBuffer->getAddress());

    context->oneTimeSubmit([&](vk::CommandBuffer commandBuffer) {
        vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
        buildRangeInfo.setPrimitiveCount(primitiveCount);
        buildRangeInfo.setPrimitiveOffset(0);
        buildRangeInfo.setFirstVertex(0);
        buildRangeInfo.setTransformOffset(0);
        commandBuffer.buildAccelerationStructuresKHR(buildGeometryInfo, &buildRangeInfo);
    });
}

void TopAccel::update(vk::CommandBuffer commandBuffer, ArrayProxy<AccelInstance> accelInstances) {
    std::vector<vk::AccelerationStructureInstanceKHR> instances;
    for (auto& instance : accelInstances) {
        instances.push_back(
            vk::AccelerationStructureInstanceKHR()
                .setTransform(toVkMatrix(instance.transform))
                .setInstanceCustomIndex(0)
                .setMask(0xFF)
                .setInstanceShaderBindingTableRecordOffset(instance.sbtOffset)
                .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable)
                .setAccelerationStructureReference(instance.bottomAccel->getBufferAddress()));
    }

    instanceBuffer->copy(instances.data());

    vk::AccelerationStructureGeometryInstancesDataKHR instancesData;
    instancesData.setArrayOfPointers(false);
    instancesData.setData(instanceBuffer->getAddress());

    vk::AccelerationStructureGeometryKHR geometry;
    geometry.setGeometryType(vk::GeometryTypeKHR::eInstances);
    geometry.setGeometry({instancesData});
    geometry.setFlags(geometryFlags);

    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
    buildGeometryInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
    buildGeometryInfo.setFlags(buildFlags);
    buildGeometryInfo.setGeometries(geometry);

    buildGeometryInfo.setMode(vk::BuildAccelerationStructureModeKHR::eUpdate);
    buildGeometryInfo.setDstAccelerationStructure(*accel);
    buildGeometryInfo.setSrcAccelerationStructure(*accel);
    buildGeometryInfo.setScratchData(scratchBuffer->getAddress());

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
}  // namespace rv
