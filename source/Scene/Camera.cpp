#include "Camera.hpp"
#include "Window/Window.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

Camera::Camera(uint32_t width, uint32_t height)
{
    aspect = static_cast<float>(width) / height;
}

glm::mat4 Camera::GetView() const
{
    return glm::lookAt(position, position + front, { 0.0f, 1.0f, 0.0f });
}

glm::mat4 Camera::GetProj() const
{
    return glm::perspective(glm::radians(fov), aspect, 0.01f, 10000.0f);
}

bool Camera::CheckDirtyAndClean()
{
    if (dirty) {
        dirty = false;
        return true;
    }
    return false;
}

void FPSCamera::ProcessInput()
{
    if (Window::MousePressed()) {
        glm::vec2 motion = Window::GetMouseMotion();
        yaw = glm::mod(yaw - motion.x * 0.1f, 360.0f);
        pitch = glm::clamp(pitch + motion.y * 0.1f, -89.9f, 89.9f);

        glm::mat4 rotation{ 1.0 };
        rotation *= glm::rotate(glm::radians(yaw), glm::vec3{ 0, 1, 0 });
        rotation *= glm::rotate(glm::radians(pitch), glm::vec3{ 1, 0, 0 });
        front = { rotation * glm::vec4{0, 0, -1, 1} };

        dirty = true;
    }

    glm::vec3 up = -glm::vec3{ 0, 1, 0 };
    glm::vec3 forward = glm::normalize(glm::vec3{ front.x, 0, front.z });
    glm::vec3 right = glm::normalize(glm::cross(up, forward));

    if (Window::MouseRightPressed()) {
        glm::vec2 motion = Window::GetMouseMotion();
        position += right * motion.x * 0.001f;
        position -= up * motion.y * 0.001f;
        dirty = true;
    }

    if (Window::KeyPressed(Key::W)) {
        position += forward * 0.15f;
        dirty = true;
    }
    if (Window::KeyPressed(Key::S)) {
        position -= forward * 0.15f;
        dirty = true;
    }
    if (Window::KeyPressed(Key::D)) {
        position += right * 0.1f;
        dirty = true;
    }
    if (Window::KeyPressed(Key::A)) {
        position -= right * 0.1f;
        dirty = true;
    }
    if (Window::KeyPressed(Key::Space)) {
        position.y -= 0.05f;
        dirty = true;
    }
}

void OrbitalCamera::ProcessInput()
{
    if (Window::MousePressed()) {
        glm::vec2 motion = Window::GetMouseMotion();

        phi = glm::mod(phi - motion.x * 0.1f, 360.0f);
        theta = std::min(std::max(theta + motion.y * 0.1f, -89.9f), 89.9f);

        glm::mat4 rotX = glm::rotate(glm::radians(theta), glm::vec3(1, 0, 0));
        glm::mat4 rotY = glm::rotate(glm::radians(phi), glm::vec3(0, 1, 0));

        position = glm::vec3(rotY * rotX * glm::vec4{ 0, 0, distance, 1 });
        front = target - position;

        dirty = true;
    }

    if (Window::MouseRightPressed()) {
        glm::vec2 motion = Window::GetMouseMotion();

        glm::vec3 up = -glm::vec3{ 0, 1, 0 };
        glm::vec3 forward = glm::normalize(glm::vec3{ front.x, 0, front.z });
        glm::vec3 right = glm::normalize(glm::cross(up, forward));

        target -= right * motion.x * 0.001f;
        target += up * motion.y * 0.001f;
        front = target - position;
        dirty = true;
    }

    if (float scroll = Window::GetMouseScroll(); scroll != 0.0) {
        if (scroll < 0.0) {
            distance = std::max(distance * 1.1f, 0.001f);
        } else {
            distance = std::max(distance * 0.9f, 0.001f);
        }

        glm::mat4 rotX = glm::rotate(glm::radians(theta), glm::vec3(1, 0, 0));
        glm::mat4 rotY = glm::rotate(glm::radians(phi), glm::vec3(0, 1, 0));
        position = glm::vec3(rotY * rotX * glm::vec4{ 0, 0, distance, 1 });
        front = target - position;
    }
}
