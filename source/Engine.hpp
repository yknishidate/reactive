#pragma once
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <spdlog/spdlog.h>
#include <memory>
#include "GUI/GUI.hpp"
#include "GUI/imgui_impl_vulkan_hpp.h"
#include "Graphics/DescriptorSet.hpp"
#include "Graphics/Image.hpp"
#include "Graphics/Pipeline.hpp"
#include "Graphics/RenderPass.hpp"
#include "Graphics/Shader.hpp"
#include "Graphics/Swapchain.hpp"
#include "Scene/AABB.hpp"
#include "Scene/Camera.hpp"
#include "Scene/Loader.hpp"
#include "Scene/Mesh.hpp"
#include "Scene/Object.hpp"
#include "Timer/CPUTimer.hpp"
#include "Timer/GPUTimer.hpp"
#include "Window/Window.hpp"

namespace Log {
using namespace spdlog;

void init() {
    spdlog::set_pattern("[%^%l%$] %v");
}
}  // namespace Log
