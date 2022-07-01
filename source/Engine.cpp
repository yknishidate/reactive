#include <random>
#include <functional>
#include "Engine.hpp"
#include "Loader.hpp"
#include "Scene.hpp"
#include "Object.hpp"
#include "UI.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

void Engine::Init()
{
    try {
        spdlog::set_pattern("[%^%l%$] %v");
        spdlog::info("Engine::Init()");
        Window::Init(1920, 1080);
        Context::Init();
        UI::Init();
    } catch (std::exception e) {
        spdlog::error(e.what());
    }
}

void Engine::Shutdown()
{
    try {
        Context::GetDevice().waitIdle();
        UI::Shutdown();
        Window::Shutdown();
    } catch (std::exception e) {
        spdlog::error(e.what());
    }
}

bool Engine::Update()
{
    bool shouldClose = Window::ShouldClose();
    Window::PollEvents();
    UI::StartFrame();
    return !shouldClose;
}

void Engine::Render(std::function<void(vk::CommandBuffer)> func)
{
    if (Window::IsMinimized()) return;
    Context::Render(func);
}
