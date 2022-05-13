#include "Engine.hpp"

int main()
{
    try {
        Engine engine;
        engine.Init();
        engine.Run();
    } catch (const std::exception& exception) {
        spdlog::error(exception.what());
    }
}
