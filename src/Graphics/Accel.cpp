#include "reactive/Graphics/Accel.hpp"

#include "reactive/Graphics/CommandBuffer.hpp"
#include "reactive/common.hpp"

namespace rv {
BottomAccel::BottomAccel(const Context& _context, const BottomAccelCreateInfo& createInfo)
    : context{&_context},
      geometryFlags{createInfo.geometryFlags},
      buildFlags{createInfo.buildFlags},
      buildType{createInfo.buildType} {
    trianglesData.setVertexFormat(vk::Format::eR32G32B32Sfloat);
    trianglesData.setVertexStride(createInfo.vertexStride);
    trianglesData.setMaxVertex(createInfo.maxVertexCount);
    trianglesData.setIndexType(vk::IndexTypeValue<uint32_t>::value);

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

    maxPrimitiveCount = createInfo.maxTriangleCount;
    auto buildSizesInfo = context->getDevice().getAccelerationStructureBuildSizesKHR(
        buildType, buildGeometryInfo, maxPrimitiveCount);

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
    if (!createInfo.debugName.empty()) {
        context->setDebugName(accel.get(), createInfo.debugName.c_str());
    }

    scratchBuffer = context->createBuffer({
        .usage = BufferUsage::Scratch,
        .memory = MemoryUsage::Host,
        .size = buildSizesInfo.buildScratchSize,
    });
}

TopAccel::TopAccel(const Context& _context, const TopAccelCreateInfo& createInfo)
    : context{&_context},
      geometryFlags{createInfo.geometryFlags},
      buildFlags{createInfo.buildFlags},
      buildType{createInfo.buildType} {
    std::vector<vk::AccelerationStructureInstanceKHR> instances;
    for (auto& instance : createInfo.accelInstances) {
        vk::AccelerationStructureInstanceKHR inst;
        inst.setTransform(toVkMatrix(instance.transform));
        inst.setInstanceCustomIndex(instance.customIndex);
        inst.setMask(0xFF);
        inst.setInstanceShaderBindingTableRecordOffset(instance.sbtOffset);
        inst.setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
        inst.setAccelerationStructureReference(instance.bottomAccel->getBufferAddress());
        instances.push_back(inst);
    }

    primitiveCount = static_cast<uint32_t>(instances.size());
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
        buildType, buildGeometryInfo, static_cast<uint32_t>(instances.size()));

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
    if (!createInfo.debugName.empty()) {
        context->setDebugName(accel.get(), createInfo.debugName.c_str());
    }

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
        inst.setInstanceCustomIndex(instance.customIndex);
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
