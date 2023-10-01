#include "Graphics/Accel.hpp"

#include "Graphics/CommandBuffer.hpp"
#include "common.hpp"

namespace rv {
BottomAccel::BottomAccel(const Context* context, BottomAccelCreateInfo createInfo)
    : context{context},
      geometryFlags{createInfo.geometryFlags},
      buildFlags{createInfo.buildFlags},
      buildType{createInfo.buildType} {
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
    geometry.setFlags(geometryFlags);

    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
    buildGeometryInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
    buildGeometryInfo.setFlags(buildFlags);
    buildGeometryInfo.setGeometries(geometry);

    primitiveCount = createInfo.triangleCount;
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
            .setType(vk::AccelerationStructureTypeKHR::eBottomLevel));

    scratchBuffer = context->createBuffer({
        .usage = BufferUsage::Scratch,
        .memory = MemoryUsage::Host,
        .size = buildSizesInfo.buildScratchSize,
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

    primitiveCount = instances.size();
    instanceBuffer = context->createBuffer({
        .usage = BufferUsage::AccelInput,
        .memory = MemoryUsage::DeviceHost,
        .size = sizeof(vk::AccelerationStructureInstanceKHR) * instances.size(),
    });
    instanceBuffer->copy(instances.data());

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

    auto buildSizesInfo = context->getDevice().getAccelerationStructureBuildSizesKHR(
        buildType, buildGeometryInfo, instances.size());

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
}

void TopAccel::updateInstances(ArrayProxy<AccelInstance> accelInstances) const {
    RV_ASSERT(primitiveCount == accelInstances.size(), "Instance count was changed. {} == {}",
              primitiveCount, accelInstances.size());

    std::vector<vk::AccelerationStructureInstanceKHR> instances;
    for (auto& instance : accelInstances) {
        vk::AccelerationStructureInstanceKHR inst;
        inst.setTransform(toVkMatrix(instance.transform));
        inst.setInstanceCustomIndex(0);
        inst.setMask(0xFF);
        inst.setInstanceShaderBindingTableRecordOffset(instance.sbtOffset);
        inst.setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
        inst.setAccelerationStructureReference(instance.bottomAccel->getBufferAddress());
        instances.push_back(inst);
    }

    // TODO: use CommandBuffer::copy()
    instanceBuffer->copy(instances.data());
}
}  // namespace rv
