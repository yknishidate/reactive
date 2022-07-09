#pragma once
#include <memory>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include "GUI/imgui_impl_vulkan_hpp.h"
#include "Vulkan/Image.hpp"
#include "Vulkan/Pipeline.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Window/Window.hpp"
#include "Scene/Mesh.hpp"
#include "Scene/Scene.hpp"
#include "GUI/GUI.hpp"

namespace Engine
{
    void Init(int width = 1920, int height = 1280);
    void Shutdown();
    bool Update();
    void Render(std::function<void()> func);
};
