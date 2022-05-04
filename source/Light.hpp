#pragma once
#include <glm/glm.hpp>

struct PointLight
{
    glm::vec3 Intensity{ 1 };
    glm::vec3 Position{ 1 };
};

struct DirectionalLight
{
    glm::vec3 Intensity{ 1 };
    glm::vec3 Direction{ 0, -1, 0 };
};
