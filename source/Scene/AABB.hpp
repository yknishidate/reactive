// #pragma once
// #include "Frustum.hpp"
//
// struct AABB {
//     AABB() = default;
//     AABB(const glm::vec3& min, const glm::vec3& max)
//         : center{(max + min) * 0.5f},
//           extents{max.x - center.x, max.y - center.y, max.z - center.z} {}
//
//     glm::vec3 center{0.f, 0.f, 0.f};
//     glm::vec3 extents{0.f, 0.f, 0.f};
//
//     bool isOnOrForwardPlane(const Plane& plane) const {
//         // Compute the projection interval radius of b onto L(t) = b.c + t * p.n
//         const float r = extents.x * std::abs(plane.normal.x) +
//                         extents.y * std::abs(plane.normal.y) + extents.z *
//                         std::abs(plane.normal.z);
//
//         return -r <= plane.getSignedDistanceToPlane(center);
//     }
//
//     bool isOnFrustum(const Frustum& camFrustum) const {
//         return (
//             isOnOrForwardPlane(camFrustum.leftFace) && isOnOrForwardPlane(camFrustum.rightFace)
//             && isOnOrForwardPlane(camFrustum.topFace) &&
//             isOnOrForwardPlane(camFrustum.bottomFace) && isOnOrForwardPlane(camFrustum.nearFace)
//             && isOnOrForwardPlane(camFrustum.farFace));
//     };
// };
