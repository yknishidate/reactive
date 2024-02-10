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
        float speed = 1.0f;
        float yaw = 0.0f;
        float pitch = 0.0f;
    };

    struct OrbitalParams {
        glm::vec3 target = {0.0f, 0.0f, 0.0f};
        float distance = 5.0f;
        float phi = 0;
        float theta = 0;
    };

    Camera() = default;

    Camera(Type type, float aspect) : type{type}, aspect{aspect} {
        if (type == Type::FirstPerson) {
            params = FirstPersonParams{};
        } else {
            params = OrbitalParams{};
        }
    }

    void processKey(int key);
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

    void setAspect(float _aspect) { aspect = _aspect; }
    void setFovY(float _fovY) { fovY = _fovY; }
    void setTarget(glm::vec3 _target);
    void setDistance(float _distance);
    void setPhi(float _phi);
    void setTheta(float _theta);

protected:
    Type type = Type::Orbital;

    float aspect = 1.0f;
    float zNear = 0.01f;
    float zFar = 10000.0f;
    float fovY = glm::radians(45.0f);
    glm::vec3 up = {0.0f, 1.0f, 0.0f};

    std::variant<FirstPersonParams, OrbitalParams> params = OrbitalParams{};
};
}  // namespace rv
