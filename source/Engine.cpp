#include <functional>
#include "Engine.hpp"
#include "Scene/Loader.hpp"

void Engine::Init(int width, int height)
{
    try {
        spdlog::set_pattern("[%^%l%$] %v");
        Window::Init(width, height);
        Context::Init();
        GUI::Init();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}

void Engine::Shutdown()
{
    try {
        Context::GetDevice().waitIdle();
        GUI::Shutdown();
        Window::Shutdown();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}

bool Engine::Update()
{
    bool shouldClose = Window::ShouldClose();
    Window::PollEvents();
    GUI::StartFrame();
    return !shouldClose;
}

void Engine::Render(std::function<void(vk::CommandBuffer)> func)
{
    if (Window::IsMinimized()) return;
    Context::WaitNextFrame();
    vk::CommandBuffer commandBuffer = Context::BeginCommandBuffer();
    func(commandBuffer);
    GUI::Render(commandBuffer);
    Context::Submit();
    Context::Present();
}
