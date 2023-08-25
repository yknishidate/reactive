#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include "App.hpp"

namespace rv {
struct Camera {
    Camera() = default;
    Camera(const App* app, uint32_t width, uint32_t height)
        : app{app}, aspect{static_cast<float>(width) / static_cast<float>(height)} {}

    virtual void processInput() = 0;
    virtual void processDragDelta(glm::vec2 dragDelta) = 0;
    virtual void processMouseScroll(float scroll) = 0;

    virtual glm::mat4 getView() const = 0;
    virtual glm::mat4 getProj() const = 0;
    virtual glm::mat4 getInvView() const;
    virtual glm::mat4 getInvProj() const;

    virtual glm::vec3 getPosition() const = 0;
    virtual glm::vec3 getFront() const = 0;
    virtual glm::vec3 getUp() const;
    virtual glm::vec3 getRight() const;

    const App* app = nullptr;
    float aspect = 1.0f;
    float zNear = 0.01f;
    float zFar = 10000.0f;
    float fovY = glm::radians(45.0f);
    glm::vec3 up = {0.0f, 1.0f, 0.0f};
};

struct FPSCamera : public Camera {
    FPSCamera() = default;
    FPSCamera(const App* app, uint32_t width, uint32_t height) : Camera{app, width, height} {}

    // Override
    void processInput() override;
    void processDragDelta(glm::vec2 dragDelta) override;
    void processMouseScroll(float scroll) override;

    glm::mat4 getView() const override;
    glm::mat4 getProj() const override;
    glm::vec3 getPosition() const override;
    glm::vec3 getFront() const override;

    glm::vec3 position = {0.0f, 0.0f, 5.0f};
    float speed = 1.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
};

struct OrbitalCamera : public Camera {
    OrbitalCamera() = default;
    OrbitalCamera(const App* app, uint32_t width, uint32_t height) : Camera{app, width, height} {}

    // Override
    void processInput() override;
    void processDragDelta(glm::vec2 dragDelta) override;
    void processMouseScroll(float scroll) override;

    glm::mat4 getView() const override;
    glm::mat4 getProj() const override;
    glm::vec3 getPosition() const override;
    glm::vec3 getFront() const override;

    glm::vec3 target = {0.0f, 0.0f, 0.0f};
    float distance = 5.0f;
    float phi = 0;
    float theta = 0;
};
}  // namespace rv
