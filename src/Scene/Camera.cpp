#include "reactive/Scene/Camera.hpp"
#include "reactive/App.hpp"
#include "reactive/Window.hpp"

namespace rv {
void Camera::processKey() {
    if (m_type == Type::FirstPerson) {
        auto& params = std::get<FirstPersonParams>(m_params);
        glm::vec3 front = getFront();
        glm::vec3 right = getRight();
        if (Window::isKeyDown(GLFW_KEY_W)) {
            params.position += front * 0.15f;
        }
        if (Window::isKeyDown(GLFW_KEY_S)) {
            params.position -= front * 0.15f;
        }
        if (Window::isKeyDown(GLFW_KEY_D)) {
            params.position += right * 0.1f;
        }
        if (Window::isKeyDown(GLFW_KEY_A)) {
            params.position -= right * 0.1f;
        }
        if (Window::isKeyDown(GLFW_KEY_SPACE)) {
            params.position += glm::vec3{0, 1, 0} * 0.05f;
        }
    }
}

void Camera::processMouseDragLeft(glm::vec2 dragDelta) {
    if (m_type == Type::FirstPerson) {
        m_eulerRotation.x = m_eulerRotation.x - dragDelta.y * 0.001f;
        m_eulerRotation.y = m_eulerRotation.y - dragDelta.x * 0.001f;
    } else {
        m_eulerRotation.x = m_eulerRotation.x - dragDelta.y * 0.002f;
        m_eulerRotation.y = m_eulerRotation.y - dragDelta.x * 0.002f;
    }
}

void Camera::processMouseDragRight(glm::vec2 dragDelta) {
    if (m_type == Type::FirstPerson) {
        auto& params = std::get<FirstPersonParams>(m_params);
        params.position += glm::vec3{0, 1, 0} * dragDelta.y * 0.01f;
    } else {
        auto& params = std::get<OrbitalParams>(m_params);
        params.target += glm::vec3{0, 1, 0} * dragDelta.y * 0.01f;
    }
}

void Camera::processMouseScroll(float scroll) {
    if (m_type == Type::FirstPerson) {
        // pass
    } else {
        auto& params = std::get<OrbitalParams>(m_params);
        params.distance = std::max(params.distance - scroll * m_dollySpeed, 0.001f);
    }
}

auto Camera::getView() const -> glm::mat4 {
    if (m_type == Type::FirstPerson) {
        auto& params = std::get<FirstPersonParams>(m_params);
        return glm::lookAt(params.position, params.position + getFront(), glm::vec3{0, 1, 0});
    } else {
        auto& params = std::get<OrbitalParams>(m_params);
        return glm::lookAt(getPosition(), params.target, glm::vec3{0, 1, 0});
    }
}

auto Camera::getProj() const -> glm::mat4 {
    return glm::perspective(m_fovY, m_aspect, m_zNear, m_zFar);
}

auto Camera::getInvView() const -> glm::mat4 {
    return glm::inverse(getView());
}

auto Camera::getInvProj() const -> glm::mat4 {
    return glm::inverse(getProj());
}

auto Camera::getPosition() const -> glm::vec3 {
    if (m_type == Type::FirstPerson) {
        auto& params = std::get<FirstPersonParams>(m_params);
        return params.position;
    } else {
        auto& params = std::get<OrbitalParams>(m_params);
        glm::mat4 rotX = glm::rotate(m_eulerRotation.x, glm::vec3(1, 0, 0));
        glm::mat4 rotY = glm::rotate(m_eulerRotation.y, glm::vec3(0, 1, 0));
        glm::vec3 position =
            params.target + glm::vec3{rotY * rotX * glm::vec4{0, 0, params.distance, 1}};
        return position;
    }
}

auto Camera::getFront() const -> glm::vec3 {
    if (m_type == Type::FirstPerson) {
        glm::mat4 rotation{1.0};
        rotation *= glm::rotate(m_eulerRotation.y, glm::vec3{0, 1, 0});
        rotation *= glm::rotate(m_eulerRotation.x, glm::vec3{1, 0, 0});
        // カメラ座標では奥が前
        return glm::normalize(glm::vec3{rotation * glm::vec4{0, 0, -1, 1}});
    } else {
        // カメラ座標では奥が前
        auto& params = std::get<OrbitalParams>(m_params);
        return glm::normalize(params.target - getPosition());
    }
}

auto Camera::getUp() const -> glm::vec3 {
    return glm::normalize(glm::cross(getRight(), getFront()));
}

auto Camera::getRight() const -> glm::vec3 {
    return glm::normalize(glm::cross(getFront(), glm::vec3{0, 1, 0}));
}

void Camera::setType(Type _type) {
    m_type = _type;
    // TODO: なるべく値を引き継ぐ
    if (m_type == Type::FirstPerson) {
        m_params = FirstPersonParams{};
    } else {
        m_params = OrbitalParams{};
    }
}

void Camera::setTarget(glm::vec3 _target) {
    assert(m_type == Type::Orbital);
    auto& params = std::get<OrbitalParams>(m_params);
    params.target = _target;
}

void Camera::setDistance(float _distance) {
    assert(m_type == Type::Orbital);
    auto& params = std::get<OrbitalParams>(m_params);
    params.distance = _distance;
}

void Camera::setPosition(glm::vec3 _position) {
    assert(m_type == Type::FirstPerson);
    auto& params = std::get<FirstPersonParams>(m_params);
    params.position = _position;
}
}  // namespace rv
