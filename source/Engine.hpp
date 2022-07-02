#pragma once
#include <memory>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>

#include "ImGui/imgui_impl_vulkan_hpp.h"
#include "Vulkan/Image.hpp"
#include "Vulkan/Pipeline.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Window.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "UI.hpp"

namespace Engine
{
    void Init();
    void Shutdown();
    bool Update();
    void Render(std::function<void(void)> func);
};
