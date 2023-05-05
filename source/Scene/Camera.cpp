#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

void FPSCamera::processInput() {
    static glm::vec2 lastCursorPos{0.0f};
    glm::vec2 cursorPos = app->getCursorPos();
    glm::vec2 cursorOffset = cursorPos - lastCursorPos;
    lastCursorPos = cursorPos;
    if (app->isMouseButtonDown(GLFW_MOUSE_BUTTON_1)) {
        yaw = glm::mod(yaw - cursorOffset.x * 0.1f, 360.0f);
        pitch = glm::clamp(pitch + cursorOffset.y * 0.1f, -89.9f, 89.9f);
    }

    glm::vec3 front = getFront();
    glm::vec3 right = glm::normalize(glm::cross(-up, front));
    if (app->isKeyDown(GLFW_KEY_W)) {
        position += front * 0.15f * speed;
    }
    if (app->isKeyDown(GLFW_KEY_S)) {
        position -= front * 0.15f * speed;
    }
    if (app->isKeyDown(GLFW_KEY_D)) {
        position += right * 0.1f * speed;
    }
    if (app->isKeyDown(GLFW_KEY_A)) {
        position -= right * 0.1f * speed;
    }
    if (app->isKeyDown(GLFW_KEY_SPACE)) {
        position.y -= 0.05f * speed;
    }
}

glm::mat4 FPSCamera::getView() const {
    return glm::lookAt(position, position + getFront(), up);
}

glm::mat4 FPSCamera::getProj() const {
    return glm::perspective(fovY, aspect, zNear, zFar);
}

glm::vec3 FPSCamera::getFront() const {
    glm::mat4 rotation{1.0};
    rotation *= glm::rotate(glm::radians(yaw), glm::vec3{0, 1, 0});
    rotation *= glm::rotate(glm::radians(pitch), glm::vec3{1, 0, 0});
    return glm::normalize(glm::vec3{rotation * glm::vec4{0, 0, -1, 1}});
}

void OrbitalCamera::processInput() {
    static glm::vec2 lastCursorPos{0.0f};
    glm::vec2 cursorPos = app->getCursorPos();
    glm::vec2 cursorOffset = cursorPos - lastCursorPos;
    lastCursorPos = cursorPos;
    if (app->isMouseButtonDown(GLFW_MOUSE_BUTTON_1)) {
        phi = glm::mod(phi - cursorOffset.x * 0.5f, 360.0f);
        theta = std::min(std::max(theta + cursorOffset.y * 0.5f, -89.9f), 89.9f);
    }

    static float lastWheel = 0.0f;
    float wheel = app->getMouseWheel().y;
    float wheelOffset = wheel - lastWheel;
    lastWheel = wheel;
    distance = std::max(distance - wheelOffset, 0.001f);
}

glm::vec3 OrbitalCamera::getPosition() const {
    glm::mat4 rotX = glm::rotate(glm::radians(theta), glm::vec3(1, 0, 0));
    glm::mat4 rotY = glm::rotate(glm::radians(phi), glm::vec3(0, 1, 0));
    return glm::vec3(rotY * rotX * glm::vec4{0, 0, distance, 1});
}
