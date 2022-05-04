#include "Camera.hpp"
#include "Window.hpp"

void Camera::Init(int width, int height)
{
    aspect = float(width) / height;
}

void Camera::ProcessInput()
{
}

glm::mat4 Camera::GetView()
{
    glm::mat4 rotation{ 1.0 };
    rotation *= glm::rotate(glm::radians(yaw), glm::vec3{ 0, 1, 0 });
    rotation *= glm::rotate(glm::radians(pitch), glm::vec3{ 1, 0, 0 });
    front = { rotation * glm::vec4{0, 0, -1, 1} };
    return glm::lookAt(position, position + front, { 0.0f, 1.0f, 0.0f });
}

glm::mat4 Camera::GetProj()
{
    return glm::perspective(glm::radians(45.0f), aspect, 0.01f, 10000.0f);
}
