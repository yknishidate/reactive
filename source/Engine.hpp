#pragma once
#include <memory>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include "GUI/imgui_impl_vulkan_hpp.h"
#include "Graphics/Image.hpp"
#include "Graphics/Pipeline.hpp"
#include "Graphics/DescriptorSet.hpp"
#include "Window/Window.hpp"
#include "Scene/Mesh.hpp"
#include "Scene/Camera.hpp"
#include "Scene/Object.hpp"
#include "GUI/GUI.hpp"

namespace Engine
{
void Init(int width = 1920, int height = 1280);
void Shutdown();
bool Update();
void Render(std::function<void(vk::CommandBuffer)> func);
};
