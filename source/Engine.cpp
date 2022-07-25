#include <functional>
#include "Engine.hpp"
#include "Scene/Loader.hpp"

void Engine::Init(int width, int height)
{
    try {
        spdlog::set_pattern("[%^%l%$] %v");
        spdlog::info("Engine::Init()");
        Window::Init(width, height);
        Graphics::Init();
        GUI::Init();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}

void Engine::Shutdown()
{
    try {
        Graphics::GetDevice().waitIdle();
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

void Engine::Render(std::function<void()> func)
{
    if (Window::IsMinimized()) return;
    Graphics::WaitNextFrame();
    Graphics::BeginCommandBuffer();
    func();
    GUI::Render(Graphics::GetCurrentCommandBuffer());
    Graphics::Submit();
    Graphics::Present();
}
