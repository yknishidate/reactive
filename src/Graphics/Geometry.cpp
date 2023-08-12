// #include "Geometry.hpp"
//
// Geometry::Geometry(vk::GeometryFlagsKHR geometryFlag, size_t primitiveCount)
//     : geometryFlag{geometryFlag}, primitiveCount{primitiveCount} {}
//
// vk::DeviceSize Geometry::getAccelSize() const {
//     auto buildType = vk::AccelerationStructureBuildTypeKHR::eDevice;
//     auto buildSizes = Context::getDevice().getAccelerationStructureBuildSizesKHR(
//         buildType, geometryInfo, primitiveCount);
//     return buildSizes.accelerationStructureSize;
// }
//
// vk::AccelerationStructureBuildGeometryInfoKHR Geometry::getInfo() const {
//     return geometryInfo;
// }
//
// Buffer Geometry::createAccelBuffer() const {
//     return {BufferUsage::AccelStorage, getAccelSize()};
// }
//
// TrianglesGeometry::TrianglesGeometry(vk::AccelerationStructureGeometryTrianglesDataKHR triangles,
//                                      vk::GeometryFlagsKHR geometryFlag,
//                                      size_t primitiveCount)
//     : Geometry{geometryFlag, primitiveCount} {
//     geometry = vk::AccelerationStructureGeometryKHR()
//                    .setGeometryType(vk::GeometryTypeKHR::eTriangles)
//                    .setGeometry({triangles})
//                    .setFlags(geometryFlag);
//
//     geometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
//                        .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
//                        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
//                        .setGeometries(geometry);
// }
//
// InstancesGeometry::InstancesGeometry(vk::AccelerationStructureGeometryInstancesDataKHR instances,
//                                      vk::GeometryFlagsKHR geometryFlag,
//                                      size_t primitiveCount)
//     : Geometry{geometryFlag, primitiveCount} {
//     update(instances);
// }
//
// void InstancesGeometry::update(vk::AccelerationStructureGeometryInstancesDataKHR instances) {
//     geometry = vk::AccelerationStructureGeometryKHR()
//                    .setGeometryType(vk::GeometryTypeKHR::eInstances)
//                    .setGeometry({instances})
//                    .setFlags(geometryFlag);
//
//     geometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
//                        .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
//                        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
//                        .setGeometries(geometry);
// }
