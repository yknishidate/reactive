#include "Engine.hpp"

int main()
{
    try {
        Engine::Init();
        Engine::Run();
        Engine::Shutdown();
    } catch (const std::exception& exception) {
        spdlog::error(exception.what());
    }
    return 0;
}
