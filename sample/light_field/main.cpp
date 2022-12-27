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
        if (!Window::mousePressed()) return;
        auto motion = Window::getMouseMotion() * 0.005f;
        if (invX) motion.x = -motion.x;
        if (invY) motion.y = -motion.y;
        uv = clamp(uv + glm::vec2(motion.x, motion.y), 0.0f, 0.99f);
    }
};

namespace fs = std::filesystem;
std::vector<std::string> GetAllFilePaths(const fs::path& directory)
{
    std::vector<std::string> imagePaths;
    try {
        for (const auto& file : fs::directory_iterator{ directory }) {
            imagePaths.push_back(file.path().string());
        }
    } catch (const std::filesystem::filesystem_error& error) {
        spdlog::error(error.what());
        return {};
    }
    return imagePaths;
}

int main()
{
    Engine::Init(1280, 1280);

    auto imagePaths = GetAllFilePaths(ASSET_DIR + "lego_lf");
    Image images{ imagePaths };
    Image outputImage{ vk::Format::eB8G8R8A8Unorm };

    ComputePipeline pipeline{};
    pipeline.loadShaders(SHADER_DIR + "light_field.comp");
    pipeline.getDescSet().record("outputImage", outputImage);
    pipeline.getDescSet().record("images", images);
    pipeline.getDescSet().setup();
    pipeline.setup(sizeof(PushConstants));

    PushConstants pushConstants;
    bool invertX = false;
    bool invertY = false;
    while (Engine::Update()) {
        static uint64_t frame = 0;
        GUI::checkbox("Invert x", invertX);
        GUI::checkbox("Invert y", invertY);
        GUI::sliderFloat("Aperture", pushConstants.apertureSize, 0.1f, 0.5f);
        GUI::sliderFloat("Focal", pushConstants.focalOffset, -0.08f, 0.06f);
        if (GUI::button("Save")) {
            outputImage.save("output_" + std::to_string(frame) + ".png");
        }

        pushConstants.HandleInput(invertX, invertY);
        Engine::Render([&](auto commandBuffer) {
            pipeline.run(commandBuffer, Window::getWidth(), Window::getHeight(), &pushConstants);
        Context::copyToBackImage(commandBuffer, outputImage); });
        frame++;
    }
    Engine::Shutdown();
}
