#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <variant>

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

    Camera(Type type, float aspect) : type{type}, aspect{aspect} {
        if (type == Type::FirstPerson) {
            params = FirstPersonParams{};
        } else {
            params = OrbitalParams{};
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

    auto getType() const -> Type { return type; }
    auto getNear() const -> float { return zNear; }
    auto getFar() const -> float { return zFar; }
    auto getAspect() const -> float { return aspect; }
    auto getFovY() const -> float { return fovY; }
    auto getEulerRotation() const -> glm::vec3 { return eulerRotation; }

    void setType(Type _type);
    void setAspect(float _aspect) { aspect = _aspect; }
    void setFovY(float _fovY) { fovY = _fovY; }
    void setTarget(glm::vec3 _target);
    void setDistance(float _distance);
    void setPosition(glm::vec3 _position);
    void setEulerRotation(glm::vec3 rotation) { eulerRotation = rotation; }

    std::optional<FirstPersonParams> getFirstPersonParams() const {
        if (type == Type::FirstPerson) {
            return std::get<FirstPersonParams>(params);
        } else {
            return std::nullopt;
        }
    }

    std::optional<OrbitalParams> getOrbitalParams() const {
        if (type == Type::Orbital) {
            return std::get<OrbitalParams>(params);
        } else {
            return std::nullopt;
        }
    }

    void setDollySpeed(float speed) { dollySpeed = speed; }

protected:
    Type type = Type::Orbital;

    // radians
    glm::vec3 eulerRotation = {0.0f, 0.0f, 0.0f};

    float aspect = 1.0f;
    float zNear = 0.1f;
    float zFar = 1000.0f;
    float fovY = glm::radians(45.0f);

    float dollySpeed = 1.0f;

    std::variant<FirstPersonParams, OrbitalParams> params = OrbitalParams{};
};
}  // namespace rv
