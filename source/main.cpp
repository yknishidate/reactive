#include "Vulkan/Vulkan.hpp"
#include "Engine.hpp"

int main()
{
    try {
        spdlog::set_pattern("[%^%l%$] %v");

        Window::Init(1920, 1080, "../asset/Vulkan.png");
        Vulkan::Init();
        Window::SetupUI();

        {
            Engine engine;
            engine.Init();
            engine.Run();
        }

        Window::Shutdown();
        Vulkan::Shutdown();
    } catch (const std::exception& exception) {
        spdlog::error(exception.what());
    }

    return 0;
}
