#include "Scene/Camera.hpp"
#include "App.hpp"
#include "Window.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

namespace rv {
void Camera::processKey() {
    if (type == Type::FirstPerson) {
        auto& _params = std::get<FirstPersonParams>(params);
        glm::vec3 front = getFront();
        glm::vec3 right = getRight();
        if (Window::isKeyDown(GLFW_KEY_W)) {
            _params.position += front * 0.15f * _params.speed;
        }
        if (Window::isKeyDown(GLFW_KEY_S)) {
            _params.position -= front * 0.15f * _params.speed;
        }
        if (Window::isKeyDown(GLFW_KEY_D)) {
            _params.position += right * 0.1f * _params.speed;
        }
        if (Window::isKeyDown(GLFW_KEY_A)) {
            _params.position -= right * 0.1f * _params.speed;
        }
        if (Window::isKeyDown(GLFW_KEY_SPACE)) {
            _params.position += glm::vec3{0, 1, 0} * 0.05f * _params.speed;
        }
    }
}

void Camera::processMouseDragLeft(glm::vec2 dragDelta) {
    if (type == Type::FirstPerson) {
        auto& _params = std::get<FirstPersonParams>(params);
        _params.yaw = _params.yaw - dragDelta.x * 0.1f;
        _params.pitch = _params.pitch + dragDelta.y * 0.1f;
    } else {
        auto& _params = std::get<OrbitalParams>(params);
        _params.phi = glm::mod(_params.phi - dragDelta.x * 0.5f, 360.0f);
        _params.theta = std::min(std::max(_params.theta + dragDelta.y * 0.5f, -89.9f), 89.9f);
    }
}

void Camera::processMouseDragRight(glm::vec2 dragDelta) {
    if (type == Type::FirstPerson) {
        auto& _params = std::get<FirstPersonParams>(params);
        _params.position += glm::vec3{0, 1, 0} * dragDelta.y * 0.01f;
    } else {
        auto& _params = std::get<OrbitalParams>(params);
        _params.target += glm::vec3{0, 1, 0} * dragDelta.y * 0.01f;
    }
}

void Camera::processMouseScroll(float scroll) {
    if (type == Type::FirstPerson) {
        // pass
    } else {
        auto& _params = std::get<OrbitalParams>(params);
        _params.distance = std::max(_params.distance - scroll, 0.001f);
    }
}

auto Camera::getView() const -> glm::mat4 {
    if (type == Type::FirstPerson) {
        auto& _params = std::get<FirstPersonParams>(params);
        return glm::lookAt(_params.position, _params.position + getFront(), glm::vec3{0, 1, 0});
    } else {
        auto& _params = std::get<OrbitalParams>(params);
        return glm::lookAt(getPosition(), _params.target, glm::vec3{0, 1, 0});
    }
}

auto Camera::getProj() const -> glm::mat4 {
    return glm::perspective(fovY, aspect, zNear, zFar);
}

auto Camera::getInvView() const -> glm::mat4 {
    return glm::inverse(getView());
}

auto Camera::getInvProj() const -> glm::mat4 {
    return glm::inverse(getProj());
}

auto Camera::getPosition() const -> glm::vec3 {
    if (type == Type::FirstPerson) {
        auto& _params = std::get<FirstPersonParams>(params);
        return _params.position;
    } else {
        auto& _params = std::get<OrbitalParams>(params);
        glm::mat4 rotX = glm::rotate(glm::radians(_params.theta), glm::vec3(1, 0, 0));
        glm::mat4 rotY = glm::rotate(glm::radians(_params.phi), glm::vec3(0, 1, 0));
        return _params.target + glm::vec3{rotY * rotX * glm::vec4{0, 0, _params.distance, 1}};
    }
}

auto Camera::getFront() const -> glm::vec3 {
    if (type == Type::FirstPerson) {
        auto& _params = std::get<FirstPersonParams>(params);
        glm::mat4 rotation{1.0};
        rotation *= glm::rotate(glm::radians(_params.yaw), glm::vec3{0, 1, 0});
        rotation *= glm::rotate(glm::radians(_params.pitch), glm::vec3{1, 0, 0});
        // カメラ座標では奥が前
        return glm::normalize(glm::vec3{rotation * glm::vec4{0, 0, -1, 1}});
    } else {
        // カメラ座標では奥が前
        auto& _params = std::get<OrbitalParams>(params);
        return glm::normalize(_params.target - getPosition());
    }
}

auto Camera::getUp() const -> glm::vec3 {
    return glm::normalize(glm::cross(getRight(), getFront()));
}

auto Camera::getRight() const -> glm::vec3 {
    return glm::normalize(glm::cross(getFront(), glm::vec3{0, 1, 0}));
}

void Camera::setType(Type _type) {
    type = _type;
    // TODO: なるべく値を引き継ぐ
    if (type == Type::FirstPerson) {
        params = FirstPersonParams{};
    } else {
        params = OrbitalParams{};
    }
}

void Camera::setTarget(glm::vec3 _target) {
    assert(type == Type::Orbital);
    auto& _params = std::get<OrbitalParams>(params);
    _params.target = _target;
}

void Camera::setDistance(float _distance) {
    assert(type == Type::Orbital);
    auto& _params = std::get<OrbitalParams>(params);
    _params.distance = _distance;
}

void Camera::setPhi(float _phi) {
    assert(type == Type::Orbital);
    auto& _params = std::get<OrbitalParams>(params);
    _params.phi = _phi;
}

void Camera::setTheta(float _theta) {
    assert(type == Type::Orbital);
    auto& _params = std::get<OrbitalParams>(params);
    _params.theta = _theta;
}

void Camera::setPosition(glm::vec3 _position) {
    assert(type == Type::FirstPerson);
    auto& _params = std::get<FirstPersonParams>(params);
    _params.position = _position;
}

void Camera::setPitch(float _pitch) {
    assert(type == Type::FirstPerson);
    auto& _params = std::get<FirstPersonParams>(params);
    _params.pitch = _pitch;
}

void Camera::setYaw(float _yaw) {
    assert(type == Type::FirstPerson);
    auto& _params = std::get<FirstPersonParams>(params);
    _params.yaw = _yaw;
}
}  // namespace rv
