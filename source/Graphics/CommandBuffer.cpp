#include "CommandBuffer.hpp"
#include "Swapchain.hpp"
#include "Context.hpp"
#include "Window/Window.hpp"
#include "Graphics/Image.hpp"
#include "Graphics/Pipeline.hpp"
#include "Scene/Mesh.hpp"
#include "GUI/GUI.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include "GUI/imgui_impl_vulkan_hpp.h"

void CommandBuffer::bindPipeline(GraphicsPipeline& pipeline)
{
    boundPipeline = &pipeline;
    pipeline.bind(commandBuffer);
}

void CommandBuffer::pushConstants(void* pushData)
{
    assert(boundPipeline);
    boundPipeline->pushConstants(commandBuffer, pushData);
}

void CommandBuffer::clearBackImage(std::array<float, 4> color)
{
    swapchain->clearBackImage(commandBuffer, color);
}

void CommandBuffer::beginRenderPass()
{
    swapchain->beginRenderPass();
}

void CommandBuffer::endRenderPass()
{
    swapchain->endRenderPass();
}

void CommandBuffer::submit()
{
    swapchain->submit();
}

void CommandBuffer::drawIndexed(Mesh& mesh)
{
    mesh.drawIndexed(commandBuffer);
}

void CommandBuffer::drawGUI(GUI& gui)
{
    gui.render(commandBuffer);
}
