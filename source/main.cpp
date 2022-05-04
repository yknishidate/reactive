#include "Vulkan.hpp"
#include "Engine.hpp"

int main()
{
    try {
        Engine engine;
        engine.Init();
        engine.Run();
        //engine.Shutdown();
        //Engine::Init();
        //Engine::Run();
        //Engine::Shutdown();
    } catch (const std::exception& exception) {
        spdlog::error(exception.what());
    }
    Window::Shutdown();
    Vulkan::Shutdown();

    return 0;
}
