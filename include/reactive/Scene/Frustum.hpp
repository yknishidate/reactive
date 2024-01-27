#pragma once
#include "Camera.hpp"

namespace rv {
struct Plane {
    Plane() = default;
    Plane(const glm::vec3& p1, const glm::vec3& norm)
        : normal{glm::normalize(norm)}, distance{glm::dot(normal, p1)} {}

    float getSignedDistanceToPlane(const glm::vec3& point) const {
        return glm::dot(normal, point) - distance;
    }

    // unit vector
    glm::vec3 normal = {0.f, 1.f, 0.f};

    // distance from origin to the nearest point in the plane
    float distance = 0.f;
};

struct Frustum {
    Frustum(const Camera& camera);

    Plane topFace;
    Plane bottomFace;

    Plane rightFace;
    Plane leftFace;

    Plane farFace;
    Plane nearFace;
};

inline Frustum::Frustum(const Camera& camera) {
    const float halfVSide = camera.getFar() * tanf(camera.getFovY() * .5f);
    const float halfHSide = halfVSide * camera.getAspect();
    const glm::vec3 position = camera.getPosition();
    const glm::vec3 front = camera.getFront();
    const glm::vec3 up = camera.getUp();
    const glm::vec3 right = camera.getRight();
    const glm::vec3 frontMultFar = camera.getFar() * front;

    nearFace = {position + camera.getNear() * front, front};
    farFace = {position + frontMultFar, -front};
    rightFace = {position, glm::cross(frontMultFar - right * halfHSide, up)};
    leftFace = {position, glm::cross(up, frontMultFar + right * halfHSide)};
    topFace = {position, glm::cross(right, frontMultFar - up * halfVSide)};
    bottomFace = {position, glm::cross(frontMultFar + up * halfVSide, right)};
}
}  // namespace rv
