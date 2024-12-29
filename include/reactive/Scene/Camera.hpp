#pragma once

#include <variant>
#include "../math.hpp"

namespace rv {
class App;

class Camera {
public:
    enum class Type {
        Orbital,
        FirstPerson,
    };

    struct FirstPersonParams {
        glm::vec3 position = {0.0f, 0.0f, 5.0f};
    };

    struct OrbitalParams {
        glm::vec3 target = {0.0f, 0.0f, 0.0f};
        float distance = 5.0f;
    };

    Camera() = default;

    Camera(Type m_type, float aspect) : m_type{m_type}, m_aspect{aspect} {
        if (m_type == Type::FirstPerson) {
            m_params = FirstPersonParams{};
        } else {
            m_params = OrbitalParams{};
        }
    }

    // キー入力は関数内で rv::Window から直接取得する
    void processKey();

    // マウス入力は rv::Window 以外で計測される場合もあるため
    // 外部から引数で受け取る
    void processMouseDragLeft(glm::vec2 dragDelta);
    void processMouseDragRight(glm::vec2 dragDelta);
    void processMouseScroll(float scroll);

    auto getView() const -> glm::mat4;
    auto getProj() const -> glm::mat4;
    auto getInvView() const -> glm::mat4;
    auto getInvProj() const -> glm::mat4;

    auto getPosition() const -> glm::vec3;
    auto getFront() const -> glm::vec3;
    auto getUp() const -> glm::vec3;
    auto getRight() const -> glm::vec3;

    auto getType() const -> Type { return m_type; }
    auto getNear() const -> float { return m_zNear; }
    auto getFar() const -> float { return m_zFar; }
    auto getAspect() const -> float { return m_aspect; }
    auto getFovY() const -> float { return m_fovY; }
    auto getEulerRotation() const -> glm::vec3 { return m_eulerRotation; }

    void setType(Type _type);
    void setAspect(float _aspect) { m_aspect = _aspect; }
    void setFovY(float _fovY) { m_fovY = _fovY; }
    void setTarget(glm::vec3 _target);
    void setDistance(float _distance);
    void setPosition(glm::vec3 _position);
    void setEulerRotation(glm::vec3 rotation) { m_eulerRotation = rotation; }

    std::optional<FirstPersonParams> getFirstPersonParams() const {
        if (m_type == Type::FirstPerson) {
            return std::get<FirstPersonParams>(m_params);
        } else {
            return std::nullopt;
        }
    }

    std::optional<OrbitalParams> getOrbitalParams() const {
        if (m_type == Type::Orbital) {
            return std::get<OrbitalParams>(m_params);
        } else {
            return std::nullopt;
        }
    }

    void setDollySpeed(float speed) { m_dollySpeed = speed; }

protected:
    Type m_type = Type::Orbital;

    // radians
    glm::vec3 m_eulerRotation = {0.0f, 0.0f, 0.0f};

    float m_aspect = 1.0f;
    float m_zNear = 0.1f;
    float m_zFar = 1000.0f;
    float m_fovY = glm::radians(45.0f);

    float m_dollySpeed = 1.0f;

    std::variant<FirstPersonParams, OrbitalParams> m_params = OrbitalParams{};
};
}  // namespace rv
