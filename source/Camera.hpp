#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

class Camera
{
public:
    void Init(int width, int height);
    void ProcessInput();
    bool CheckDirtyAndClean();
    glm::mat4 GetView() const;
    glm::mat4 GetProj() const;

private:
    glm::vec3 position = { 0.0f, 0.0f, 5.0f };
    glm::vec3 front = { 0.0f, 0.0f, -1.0f };
    float aspect = 1.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    bool dirty = false;
};
