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
    glm::vec3 getFront() const { return glm::normalize(front); }
    glm::vec3 getUp() const { return glm::vec3{0, 1, 0}; }
    glm::vec3 getRight() const { return glm::normalize(glm::cross(-getUp(), getFront())); }
    glm::vec3 getPosition() const { return position; }
    glm::mat4 getView() const;
    glm::mat4 getProj() const;
    glm::mat4 getInvView() const { return glm::inverse(getView()); }
    glm::mat4 getInvProj() const { return glm::inverse(getProj()); }
    float getFovY() const { return fovY; }
    float getAspect() const { return aspect; }
    float getNear() const { return zNear; }
    float getFar() const { return zFar; }

    void setPosition(float x, float y, float z) { position = {x, y, z}; }
    void setPosition(glm::vec3 pos) { position = pos; }
    void setSpeed(float s) { speed = s; }
    void setPitch(float p) {
        pitch = p;
        updateFront();
    }
    void setYaw(float y) {
        yaw = y;
        updateFront();
    }
    void setNear(float n) { zNear = n; }
    void setFar(float f) { zFar = f; }
    void setFovY(float f) { fovY = f; }

private:
    void updateFront();
    float speed = 1.0f;
    glm::vec3 position = {0.0f, 0.0f, 5.0f};
    glm::vec3 front = {0.0f, 0.0f, -1.0f};
    float aspect = 1.0f;
    float zNear = 0.01f;
    float zFar = 10000.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    bool dirty = false;
    float fovY = glm::radians(45.0f);
};
