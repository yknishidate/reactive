#include "Vulkan/Vulkan.hpp"
#include "Engine.hpp"

int main()
{
    try {
        spdlog::set_pattern("[%^%l%$] %v");

        Window::Init(1920, 1080, "../asset/Vulkan.png");
        Vulkan::Init();
        Vulkan::SetupUI();

        {
            Engine engine;
            engine.Init();
            engine.Run();
        }

        Vulkan::Shutdown();
        Window::Shutdown();
    } catch (const std::exception& exception) {
        spdlog::error(exception.what());
    }

    return 0;
}
