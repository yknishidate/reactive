#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// TODO: add orbital camera
class Camera {
public:
    Camera() = default;
    Camera(uint32_t width, uint32_t height);
    void processInput();
    bool checkDirtyAndClean();
    glm::vec3 getPosition() const { return position; }
    glm::mat4 getView() const;
    glm::mat4 getProj() const;
    glm::mat4 getInvView() const { return glm::inverse(getView()); }
    glm::mat4 getInvProj() const { return glm::inverse(getProj()); }
    void setPosition(float x, float y, float z) { position = {x, y, z}; }
    void setSpeed(float s) { speed = s; }

private:
    float speed = 1.0f;
    glm::vec3 position = {0.0f, 0.0f, 5.0f};
    glm::vec3 front = {0.0f, 0.0f, -1.0f};
    float aspect = 1.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    bool moved = false;
};
