#include "Scene/Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

namespace rv {
auto Camera::getInvView() const -> glm::mat4 {
    return glm::inverse(getView());
}

auto Camera::getInvProj() const -> glm::mat4 {
    return glm::inverse(getProj());
}

auto Camera::getUp() const -> glm::vec3 {
    return up;
}

auto Camera::getRight() const -> glm::vec3 {
    return glm::normalize(glm::cross(-getUp(), getFront()));
}

auto FPSCamera::processInput() -> bool {
    if (!app) {
        return false;
    }

    bool updated = false;
    static glm::vec2 lastCursorPos{0.0f};
    glm::vec2 cursorPos = app->getCursorPos();
    glm::vec2 cursorOffset = cursorPos - lastCursorPos;
    lastCursorPos = cursorPos;
    if (app->isMouseButtonDown(GLFW_MOUSE_BUTTON_1)) {
        if (cursorOffset != glm::vec2(0.0)) {
            updated = true;
        }

        processDragDelta(cursorOffset);
    }

    glm::vec3 front = getFront();
    glm::vec3 right = glm::normalize(glm::cross(-up, front));
    if (app->isKeyDown(GLFW_KEY_W)) {
        position += front * 0.15f * speed;
        updated = true;
    }
    if (app->isKeyDown(GLFW_KEY_S)) {
        position -= front * 0.15f * speed;
        updated = true;
    }
    if (app->isKeyDown(GLFW_KEY_D)) {
        position += right * 0.1f * speed;
        updated = true;
    }
    if (app->isKeyDown(GLFW_KEY_A)) {
        position -= right * 0.1f * speed;
        updated = true;
    }
    if (app->isKeyDown(GLFW_KEY_SPACE)) {
        position.y -= 0.05f * speed;
        updated = true;
    }
    return updated;
}

void FPSCamera::processDragDelta(glm::vec2 dragDelta) {
    yaw = yaw - dragDelta.x * 0.1f;
    pitch = pitch + dragDelta.y * 0.1f;
}

void FPSCamera::processMouseScroll(float scroll) {}

auto FPSCamera::getView() const -> glm::mat4 {
    return glm::lookAt(position, position + getFront(), up);
}

auto FPSCamera::getProj() const -> glm::mat4 {
    return glm::perspective(fovY, aspect, zNear, zFar);
}

auto FPSCamera::getPosition() const -> glm::vec3 {
    return position;
}

auto FPSCamera::getFront() const -> glm::vec3 {
    glm::mat4 rotation{1.0};
    rotation *= glm::rotate(glm::radians(yaw), glm::vec3{0, 1, 0});
    rotation *= glm::rotate(glm::radians(pitch), glm::vec3{1, 0, 0});
    return glm::normalize(glm::vec3{rotation * glm::vec4{0, 0, -1, 1}});
}

auto OrbitalCamera::processInput() -> bool {
    if (!app) {
        return false;
    }

    bool updated = false;
    static glm::vec2 lastCursorPos{0.0f};
    glm::vec2 cursorPos = app->getCursorPos();
    glm::vec2 cursorOffset = cursorPos - lastCursorPos;
    lastCursorPos = cursorPos;
    if (app->isMouseButtonDown(GLFW_MOUSE_BUTTON_1)) {
        if (cursorOffset != glm::vec2(0.0)) {
            updated = true;
        }

        processDragDelta(cursorOffset);
    }

    static float lastWheel = 0.0f;
    float wheel = app->getMouseWheel().y;
    float wheelOffset = wheel - lastWheel;
    lastWheel = wheel;
    distance = std::max(distance - wheelOffset, 0.001f);
    if (wheelOffset != 0.0f) {
        updated = true;
    }
    return updated;
}

void OrbitalCamera::processDragDelta(glm::vec2 dragDelta) {
    phi = glm::mod(phi - dragDelta.x * 0.5f, 360.0f);
    theta = std::min(std::max(theta + dragDelta.y * 0.5f, -89.9f), 89.9f);
}

void OrbitalCamera::processMouseScroll(float scroll) {
    distance = std::max(distance - scroll, 0.001f);
}

auto OrbitalCamera::getView() const -> glm::mat4 {
    return glm::lookAt(getPosition(), target, up);
}

auto OrbitalCamera::getProj() const -> glm::mat4 {
    return glm::perspective(fovY, aspect, zNear, zFar);
}

auto OrbitalCamera::getPosition() const -> glm::vec3 {
    glm::mat4 rotX = glm::rotate(glm::radians(theta), glm::vec3(1, 0, 0));
    glm::mat4 rotY = glm::rotate(glm::radians(phi), glm::vec3(0, 1, 0));
    return glm::vec3(rotY * rotX * glm::vec4{0, 0, distance, 1});
}

auto OrbitalCamera::getFront() const -> glm::vec3 {
    return glm::normalize(getPosition() - target);
}
}  // namespace rv
