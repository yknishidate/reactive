#pragma once
#include "Window.hpp"
#include <glm/glm.hpp>

namespace Key
{
    constexpr inline int W = GLFW_KEY_W;
    constexpr inline int A = GLFW_KEY_A;
    constexpr inline int S = GLFW_KEY_S;
    constexpr inline int D = GLFW_KEY_D;
    constexpr inline int Space = GLFW_KEY_SPACE;
}

class Input
{
public:
    static void Update();
    static bool MousePressed();
    static bool KeyPressed(int key);
    static glm::vec2 GetMousePos() { return currMousePos; }
    static glm::vec2 GetMouseMotion() { return currMousePos - lastMousePos; }

private:
    static inline glm::vec2 currMousePos = { 0.0f, 0.0f };
    static inline glm::vec2 lastMousePos = { 0.0f, 0.0f };
};
