#pragma once
#include <imgui.h>
#include <glm/glm.hpp>

class Swapchain;

namespace GUI {
void init(Swapchain& swapchain);
void shutdown();
void startFrame();
void render(vk::CommandBuffer commandBuffer);
};  // namespace GUI
