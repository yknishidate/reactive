#include "Engine.hpp"

struct PushConstants
{
    int rows = 17;
    int cols = 17;
    glm::vec2 uv = { 0.5, 0.5 };
    float apertureSize = 0.1f;
    float focalOffset = 0.0f;

    void HandleInput(bool invX, bool invY)
    {
        if (!Window::MousePressed()) return;
        auto motion = Window::GetMouseMotion() * 0.005f;
        if (invX) motion.x = -motion.x;
        if (invY) motion.y = -motion.y;
        uv = clamp(uv + glm::vec2(motion.x, motion.y), 0.0f, 0.99f);
    }
};

namespace fs = std::filesystem;
std::vector<std::string> GetAllFilePaths(const fs::path& directory)
{
    std::vector<std::string> imagePaths;
    for (const auto& file : fs::directory_iterator{ directory }) {
        imagePaths.push_back(file.path().string());
    }
    return imagePaths;
}

int main()
{
    Engine::Init(1280, 1280);

    auto imagePaths = GetAllFilePaths(ASSET_DIR + "lego_lf");
    Image images{ imagePaths };
    Image outputImage{ vk::Format::eB8G8R8A8Unorm };

    ComputePipeline pipeline{ };
    pipeline.LoadShaders(SHADER_DIR + "light_field.comp");
    pipeline.GetDescSet().Register("outputImage", outputImage);
    pipeline.GetDescSet().Register("images", images);
    pipeline.GetDescSet().Setup();
    pipeline.Setup(sizeof(PushConstants));

    PushConstants pushConstants;
    bool invertX = false;
    bool invertY = false;
    while (Engine::Update()) {
        static uint64_t frame = 0;
        GUI::Checkbox("Invert x", invertX);
        GUI::Checkbox("Invert y", invertY);
        GUI::SliderFloat("Aperture", pushConstants.apertureSize, 0.1f, 0.5f);
        GUI::SliderFloat("Focal", pushConstants.focalOffset, -0.08f, 0.06f);
        if (GUI::Button("Save")) {
            outputImage.Save("output_" + std::to_string(frame) + ".png");
        }

        pushConstants.HandleInput(invertX, invertY);
        Engine::Render([&]() {
            pipeline.Run(&pushConstants);
            outputImage.CopyToBackImage(); });
        frame++;
    }
    Engine::Shutdown();
}
