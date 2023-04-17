// #include "Accel.hpp"
// #include "Context.hpp"
// #include "Scene/Object.hpp"
//
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
//
// BottomAccel::BottomAccel(const Mesh& mesh, vk::GeometryFlagBitsKHR geometryFlag) {
//     size_t primitiveCount = mesh.getTriangleCount();
//     TrianglesGeometry geometry{mesh.getTriangles(), geometryFlag, primitiveCount};
//     auto size = geometry.getAccelSize();
//     auto type = vk::AccelerationStructureTypeKHR::eBottomLevel;
//
//     buffer = geometry.createAccelBuffer();
//     accel = createAccel(buffer.getBuffer(), size, type);
//     buildAccel(*accel, size, primitiveCount, geometry.getInfo());
// }
//
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
