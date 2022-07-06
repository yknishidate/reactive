#include <functional>
#include "Engine.hpp"
#include "Scene/Loader.hpp"

void Engine::Init()
{
    try {
        spdlog::set_pattern("[%^%l%$] %v");
        spdlog::info("Engine::Init()");
        Window::Init(1920, 1080);
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

void Engine::Render(std::function<void()> func)
{
    if (Window::IsMinimized()) return;
    Context::WaitNextFrame();
    Context::BeginCommandBuffer();
    func();
    GUI::Render(Context::GetCurrentCommandBuffer());
    Context::Submit();
    Context::Present();
}
