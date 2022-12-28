#include "Engine.hpp"

struct PushConstants {
    int rows = 17;
    int cols = 17;
    glm::vec2 uv = {0.5, 0.5};
    float apertureSize = 0.1f;
    float focalOffset = 0.0f;

    void HandleInput(bool invX, bool invY) {
        if (!Window::mousePressed())
            return;
        auto motion = Window::getMouseMotion() * 0.005f;
        if (invX) {
            motion.x = -motion.x;
        }
        if (invY) {
            motion.y = -motion.y;
        }
        uv = clamp(uv + glm::vec2(motion.x, motion.y), 0.0f, 0.99f);
    }
};

namespace fs = std::filesystem;
std::vector<std::string> GetAllFilePaths(const fs::path& directory) {
    std::vector<std::string> imagePaths;
    try {
        for (const auto& file : fs::directory_iterator{directory}) {
            imagePaths.push_back(file.path().string());
        }
    } catch (const std::filesystem::filesystem_error& error) {
        spdlog::error(error.what());
        return {};
    }
    return imagePaths;
}

int main() {
    try {
        Log::init();
        Window::init(1024, 1024);
        Context::init();

        Swapchain swapchain{};
        GUI gui{swapchain};

        auto imagePaths = GetAllFilePaths(ASSET_DIR + "rectified");
        Image images{imagePaths};
        Image outputImage{vk::Format::eB8G8R8A8Unorm};

        Shader compShader{SHADER_DIR + "light_field.comp"};

        DescriptorSet descSet;
        descSet.addResources(compShader);
        descSet.record("outputImage", outputImage);
        descSet.record("images", images);
        descSet.allocate();

        ComputePipeline pipeline{descSet};
        pipeline.setComputeShader(compShader);
        pipeline.setup(sizeof(PushConstants));

        PushConstants pushConstants;
        bool invertX = false;
        bool invertY = false;
        while (!Window::shouldClose()) {
            Window::pollEvents();

            static uint64_t frame = 0;
            gui.startFrame();
            gui.checkbox("Invert x", invertX);
            gui.checkbox("Invert y", invertY);
            gui.sliderFloat("Aperture", pushConstants.apertureSize, 0.1f, 0.5f);
            gui.sliderFloat("Focal", pushConstants.focalOffset, -0.08f, 0.06f);
            if (gui.button("Save")) {
                outputImage.save("output_" + std::to_string(frame) + ".png");
            }

            pushConstants.HandleInput(invertX, invertY);

            swapchain.waitNextFrame();

            CommandBuffer commandBuffer = swapchain.beginCommandBuffer();
            commandBuffer.bindPipeline(pipeline);
            commandBuffer.pushConstants(pipeline, &pushConstants);
            commandBuffer.dispatch(pipeline, Window::getWidth(), Window::getHeight());
            commandBuffer.copyToBackImage(outputImage);
            commandBuffer.beginRenderPass();
            commandBuffer.drawGUI(gui);
            commandBuffer.endRenderPass();
            commandBuffer.submit();

            swapchain.present();
            frame++;
        }
        Context::waitIdle();
        Window::shutdown();
    } catch (const std::exception& e) {
        Log::error(e.what());
    }
}
