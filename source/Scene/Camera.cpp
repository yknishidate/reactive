#include "Camera.hpp"
#include "Window/Window.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

Camera::Camera(uint32_t width, uint32_t height)
{
    aspect = static_cast<float>(width) / height;
}

void Camera::processInput()
{
    if (Window::mousePressed()) {
        glm::vec2 motion = Window::getMouseMotion();
        yaw = glm::mod(yaw - motion.x * 0.1f, 360.0f);
        pitch = glm::clamp(pitch + motion.y * 0.1f, -89.9f, 89.9f);

        glm::mat4 rotation{ 1.0 };
        rotation *= glm::rotate(glm::radians(yaw), glm::vec3{ 0, 1, 0 });
        rotation *= glm::rotate(glm::radians(pitch), glm::vec3{ 1, 0, 0 });
        front = { rotation * glm::vec4{0, 0, -1, 1} };

        dirty = true;
    }

    glm::vec3 forward = glm::normalize(glm::vec3{ front.x, 0, front.z });
    glm::vec3 right = glm::normalize(glm::cross(-glm::vec3{ 0, 1, 0 }, forward));

    if (Window::keyPressed(Key::W)) {
        position += forward * 0.15f;
        dirty = true;
    }
    if (Window::keyPressed(Key::S)) {
        position -= forward * 0.15f;
        dirty = true;
    }
    if (Window::keyPressed(Key::D)) {
        position += right * 0.1f;
        dirty = true;
    }
    if (Window::keyPressed(Key::A)) {
        position -= right * 0.1f;
        dirty = true;
    }
    if (Window::keyPressed(Key::Space)) {
        position.y -= 0.05f;
        dirty = true;
    }
}

glm::mat4 Camera::getView() const
{
    return glm::lookAt(position, position + front, { 0.0f, 1.0f, 0.0f });
}

glm::mat4 Camera::getProj() const
{
    return glm::perspective(glm::radians(45.0f), aspect, 0.01f, 10000.0f);
}

bool Camera::checkDirtyAndClean()
{
    if (dirty) {
        dirty = false;
        return true;
    }
    return false;
}
