#pragma once
#include <glm/glm.hpp>

struct SphereLight
{
    glm::vec3 intensity{1.0};
    glm::vec3 position{0.0};
    float radius{1.0};
};

struct PointLight
{
    glm::vec3 intensity{1.0};
    glm::vec3 position{0.0};
};

struct DirectionalLight
{
    glm::vec3 intensity{1.0};
    glm::vec3 direction{1.0};
};
