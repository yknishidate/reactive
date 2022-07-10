#include "Engine.hpp"

struct PushConstants
{
    int rows = 1;
    int cols = 1;
};

namespace fs = std::filesystem;
std::vector<Image> LoadImages(fs::path directory)
{
    size_t imageCount = std::distance(fs::directory_iterator{ directory }, fs::directory_iterator{});

    std::vector<Image> images;
    images.reserve(imageCount);

    for (const auto& file : fs::directory_iterator{ directory }) {
        std::cout << file.path() << std::endl;
        images.emplace_back(file.path().string());
    }
    return images;
}

int main()
{
    Engine::Init(750, 750);

    Image outputImage{ vk::Format::eB8G8R8A8Unorm };

    ComputePipeline pipeline{ };
    pipeline.LoadShaders(SHADER_DIR + "light_field.comp");
    pipeline.GetDescSet().Register("outputImage", outputImage);
    pipeline.GetDescSet().Setup();
    pipeline.Setup(sizeof(PushConstants));

    std::vector<Image> images = LoadImages({ ASSET_DIR + "chess_lf" });

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
