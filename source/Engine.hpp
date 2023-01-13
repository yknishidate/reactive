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
#include "Graphics/Shader.hpp"
#include "Scene/Camera.hpp"
#include "Scene/Loader.hpp"
#include "Scene/Mesh.hpp"
#include "Scene/Object.hpp"
#include "Window/Window.hpp"

namespace Log {
using namespace spdlog;

void init() {
    spdlog::set_pattern("[%^%l%$] %v");
}
}  // namespace Log
