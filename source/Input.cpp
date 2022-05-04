#include "Input.hpp"
#include "imgui.h"

void Input::Update()
{
    lastMousePos = currMousePos;

    double xpos, ypos;
    glfwGetCursorPos(Window::GetWindow(), &xpos, &ypos);
    currMousePos = { xpos, ypos };
}

bool Input::MousePressed()
{
    bool pressed = glfwGetMouseButton(Window::GetWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    return  pressed && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
}

bool Input::KeyPressed(int key)
{
    return glfwGetKey(Window::GetWindow(), key) == GLFW_PRESS;
}
