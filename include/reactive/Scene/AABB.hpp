#pragma once
#include "Frustum.hpp"

namespace rv {
struct AABB {
    AABB() = default;
    AABB(const glm::vec3& min, const glm::vec3& max)
        : center{(max + min) * 0.5f},
          extents{max.x - center.x, max.y - center.y, max.z - center.z} {}

    glm::vec3 center{0.f, 0.f, 0.f};
    glm::vec3 extents{0.f, 0.f, 0.f};

    bool isOnOrForwardPlane(const Plane& plane) const {
        // Compute the projection interval radius of b onto L(t) = b.c + t * p.n
        const float r = extents.x * std::abs(plane.normal.x) +
                        extents.y * std::abs(plane.normal.y) + extents.z * std::abs(plane.normal.z);

        return -r <= plane.getSignedDistanceToPlane(center);
    }

    bool isOnFrustum(const Frustum& camFrustum) const {
        return (
            isOnOrForwardPlane(camFrustum.leftFace) && isOnOrForwardPlane(camFrustum.rightFace) &&
            isOnOrForwardPlane(camFrustum.topFace) && isOnOrForwardPlane(camFrustum.bottomFace) &&
            isOnOrForwardPlane(camFrustum.nearFace) && isOnOrForwardPlane(camFrustum.farFace));
    }

    std::vector<glm::vec3> getCorners() const {
        std::vector<glm::vec3> corners(8);
        corners[0] = center + glm::vec3(-extents.x, -extents.y, -extents.z);
        corners[1] = center + glm::vec3(extents.x, -extents.y, -extents.z);
        corners[2] = center + glm::vec3(extents.x, extents.y, -extents.z);
        corners[3] = center + glm::vec3(-extents.x, extents.y, -extents.z);
        corners[4] = center + glm::vec3(-extents.x, -extents.y, extents.z);
        corners[5] = center + glm::vec3(extents.x, -extents.y, extents.z);
        corners[6] = center + glm::vec3(extents.x, extents.y, extents.z);
        corners[7] = center + glm::vec3(-extents.x, extents.y, extents.z);
        return corners;
    }

    glm::vec3 getFurthestCorner(const glm::vec3& dir) const {
        std::vector<glm::vec3> corners = getCorners();
        glm::vec3 furthestCorner = corners[0];
        float maxDistance = glm::dot(corners[0], dir);

        for (const auto& corner : corners) {
            float distance = glm::dot(corner, dir);
            if (distance > maxDistance) {
                maxDistance = distance;
                furthestCorner = corner;
            }
        }

        return furthestCorner;
    }

    static AABB merge(const AABB& a, const AABB& b) {
        if (a.center == glm::vec3{0.0f, 0.0f, 0.0f} && a.extents == glm::vec3{0.0f, 0.0f, 0.0f}) {
            return b;
        }
        if (b.center == glm::vec3{0.0f, 0.0f, 0.0f} && b.extents == glm::vec3{0.0f, 0.0f, 0.0f}) {
            return a;
        }
        glm::vec3 minA = a.center - a.extents;
        glm::vec3 maxA = a.center + a.extents;
        glm::vec3 minB = b.center - b.extents;
        glm::vec3 maxB = b.center + b.extents;

        glm::vec3 min = glm::min(minA, minB);
        glm::vec3 max = glm::max(maxA, maxB);
        return {min, max};
    }
};
}  // namespace rv
