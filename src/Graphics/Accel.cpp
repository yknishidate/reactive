#include "Graphics/Accel.hpp"

#include "Graphics/CommandBuffer.hpp"

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

    context->oneTimeSubmit([&](CommandBufferHandle commandBuffer) {
        vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
        buildRangeInfo.setPrimitiveCount(primitiveCount);
        buildRangeInfo.setPrimitiveOffset(0);
        buildRangeInfo.setFirstVertex(0);
        buildRangeInfo.setTransformOffset(0);
        commandBuffer->commandBuffer->buildAccelerationStructuresKHR(buildGeometryInfo,
                                                                     &buildRangeInfo);
    });
}

TopAccel::TopAccel(const Context* context, TopAccelCreateInfo createInfo)
    : context{context},
      geometryFlags{createInfo.geometryFlags},
      buildFlags{createInfo.buildFlags},
      buildType{createInfo.buildType} {
    std::vector<vk::AccelerationStructureInstanceKHR> instances;
    for (auto& instance : createInfo.accelInstances) {
        vk::AccelerationStructureInstanceKHR inst;
        inst.setTransform(toVkMatrix(instance.transform));
        inst.setInstanceCustomIndex(0);
        inst.setMask(0xFF);
        inst.setInstanceShaderBindingTableRecordOffset(instance.sbtOffset);
        inst.setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
        inst.setAccelerationStructureReference(instance.bottomAccel->getBufferAddress());
        instances.push_back(inst);
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

    context->oneTimeSubmit([&](CommandBufferHandle commandBuffer) {
        vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
        buildRangeInfo.setPrimitiveCount(primitiveCount);
        buildRangeInfo.setPrimitiveOffset(0);
        buildRangeInfo.setFirstVertex(0);
        buildRangeInfo.setTransformOffset(0);
        commandBuffer->commandBuffer->buildAccelerationStructuresKHR(buildGeometryInfo,
                                                                     &buildRangeInfo);
    });
}
}  // namespace rv
