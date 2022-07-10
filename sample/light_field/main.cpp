#include "Engine.hpp"

struct PushConstants
{
    int rows = 1;
    int cols = 1;
};

namespace fs = std::filesystem;

int main()
{
    Engine::Init(750, 750);

    Image outputImage{ vk::Format::eB8G8R8A8Unorm };

    ComputePipeline pipeline{ };
    pipeline.LoadShaders(SHADER_DIR + "light_field.comp");
    pipeline.GetDescSet().Register("outputImage", outputImage);
    pipeline.GetDescSet().Setup();
    pipeline.Setup(sizeof(PushConstants));

    fs::path directory{ ASSET_DIR + "chess_lf" };
    std::vector<std::string> imagePaths;
    for (const auto& file : fs::directory_iterator{ directory }) {
        imagePaths.push_back(file.path().string());
        //    int row, col;
        //    double camx, camy;
        //    char ext[64];
        //    std::sscanf(file.path().filename().string().c_str(),
        //                "out_%d_%d_%lf_%lf%s", &row, &col, &camx, &camy, ext);
    }
    Image image{ imagePaths };

    while (Engine::Update()) {
        PushConstants pushConstants;
        pushConstants.rows = 1;
        pushConstants.cols = 1;

        Engine::Render([&]() {
            pipeline.Run(&pushConstants);
            outputImage.CopyToBackImage(); });
    }
    Engine::Shutdown();
}
