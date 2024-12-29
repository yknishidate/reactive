#include "reactive/Graphics/Accel.hpp"

#include "reactive/Graphics/CommandBuffer.hpp"
#include "reactive/common.hpp"

namespace rv {
BottomAccel::BottomAccel(const Context& context, const BottomAccelCreateInfo& createInfo)
    : m_context{&context},
      m_geometryFlags{createInfo.geometryFlags},
      m_buildFlags{createInfo.buildFlags},
      m_buildType{createInfo.buildType} {
    m_trianglesData.setVertexFormat(vk::Format::eR32G32B32Sfloat);
    m_trianglesData.setVertexStride(createInfo.vertexStride);
    m_trianglesData.setMaxVertex(createInfo.maxVertexCount);
    m_trianglesData.setIndexType(vk::IndexTypeValue<uint32_t>::value);

    vk::AccelerationStructureGeometryDataKHR geometryData;
    geometryData.setTriangles(m_trianglesData);

    vk::AccelerationStructureGeometryKHR geometry;
    geometry.setGeometryType(vk::GeometryTypeKHR::eTriangles);
    geometry.setGeometry({geometryData});
    geometry.setFlags(m_geometryFlags);

    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
    buildGeometryInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
    buildGeometryInfo.setFlags(m_buildFlags);
    buildGeometryInfo.setGeometries(geometry);

    m_maxPrimitiveCount = createInfo.maxTriangleCount;
    auto buildSizesInfo = m_context->getDevice().getAccelerationStructureBuildSizesKHR(
        m_buildType, buildGeometryInfo, m_maxPrimitiveCount);

    m_buffer = m_context->createBuffer({
        .usage = BufferUsage::AccelStorage,
        .memory = MemoryUsage::Device,
        .size = buildSizesInfo.accelerationStructureSize,
    });

    m_accel = m_context->getDevice().createAccelerationStructureKHRUnique(
        vk::AccelerationStructureCreateInfoKHR{}
            .setBuffer(m_buffer->getBuffer())
            .setSize(buildSizesInfo.accelerationStructureSize)
            .setType(vk::AccelerationStructureTypeKHR::eBottomLevel));
    if (!createInfo.debugName.empty()) {
        m_context->setDebugName(m_accel.get(), createInfo.debugName.c_str());
    }

    m_scratchBuffer = m_context->createBuffer({
        .usage = BufferUsage::Scratch,
        .memory = MemoryUsage::Host,
        .size = buildSizesInfo.buildScratchSize,
    });
}

TopAccel::TopAccel(const Context& context, const TopAccelCreateInfo& createInfo)
    : m_context{&context},
      m_geometryFlags{createInfo.geometryFlags},
      m_buildFlags{createInfo.buildFlags},
      m_buildType{createInfo.buildType} {
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

    m_primitiveCount = static_cast<uint32_t>(instances.size());
    m_instanceBuffer = m_context->createBuffer({
        .usage = BufferUsage::AccelInput,
        .memory = MemoryUsage::DeviceHost,
        .size = sizeof(vk::AccelerationStructureInstanceKHR) * instances.size(),
    });
    m_instanceBuffer->copy(instances.data());

    m_instancesData.setArrayOfPointers(false);
    m_instancesData.setData(m_instanceBuffer->getAddress());

    vk::AccelerationStructureGeometryKHR geometry;
    geometry.setGeometryType(vk::GeometryTypeKHR::eInstances);
    geometry.setGeometry({m_instancesData});
    geometry.setFlags(m_geometryFlags);

    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
    buildGeometryInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
    buildGeometryInfo.setFlags(m_buildFlags);
    buildGeometryInfo.setGeometries(geometry);

    auto buildSizesInfo = m_context->getDevice().getAccelerationStructureBuildSizesKHR(
        m_buildType, buildGeometryInfo, static_cast<uint32_t>(instances.size()));

    m_buffer = m_context->createBuffer({
        .usage = BufferUsage::AccelStorage,
        .memory = MemoryUsage::Device,
        .size = buildSizesInfo.accelerationStructureSize,
    });

    m_accel = m_context->getDevice().createAccelerationStructureKHRUnique(
        vk::AccelerationStructureCreateInfoKHR{}
            .setBuffer(m_buffer->getBuffer())
            .setSize(buildSizesInfo.accelerationStructureSize)
            .setType(vk::AccelerationStructureTypeKHR::eTopLevel));
    if (!createInfo.debugName.empty()) {
        m_context->setDebugName(m_accel.get(), createInfo.debugName.c_str());
    }

    m_scratchBuffer = m_context->createBuffer({
        .usage = BufferUsage::Scratch,
        .memory = MemoryUsage::Device,
        .size = buildSizesInfo.buildScratchSize,
    });
}

void TopAccel::updateInstances(ArrayProxy<AccelInstance> accelInstances) const {
    RV_ASSERT(m_primitiveCount == accelInstances.size(), "Instance count was changed. {} == {}",
              m_primitiveCount, accelInstances.size());

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
    m_instanceBuffer->copy(instances.data());
}
}  // namespace rv
