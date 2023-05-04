#include "App.hpp"

struct PushConstants {
    int frame = 0;
};

class HelloApp : public App {
public:
    HelloApp()
        : App({
              .windowWidth = 1280,
              .windowHeight = 720,
              .title = "HelloCompute",
              .enableRayTracing = true,
          }) {}

    void onStart() override {
        image = context.createImage({
            .usage = ImageUsage::GeneralStorage,
            .width = width,
            .height = height,
        });

        std::string compCode = File::readFile(SHADER_DIR + "hello_compute.comp");

        compShader = context.createShader({
            .glslCode = compCode,
            .shaderStage = vk::ShaderStageFlagBits::eCompute,
        });

        descSet = context.createDescriptorSet({
            .shaders = {&compShader},
            .images = {{"outputImage", image}},
        });

        pipeline = context.createComputePipeline({
            .computeShader = &compShader,
            .descSetLayout = descSet.getLayout(),
            .pushSize = sizeof(PushConstants),
        });
    }

    void onUpdate() override { pushConstants.frame++; }

    void onRender(const CommandBuffer& commandBuffer) override {
        ImGui::SliderInt("Test slider", &testInt, 0, 100);

        commandBuffer.bindDescriptorSet(descSet, pipeline);
        commandBuffer.bindPipeline(pipeline);
        commandBuffer.pushConstants(pipeline, &pushConstants);
        commandBuffer.dispatch(pipeline, width, height, 1);
        commandBuffer.copyImageToImage(image.getImage(), getBackImage(),  // images
                                       vk::ImageLayout::eGeneral, vk::ImageLayout::ePresentSrcKHR,
                                       width, height);
    }

    Image image;

    Shader compShader;
    DescriptorSet descSet;
    ComputePipeline pipeline;

    PushConstants pushConstants;
    int testInt = 0;
};

int main() {
    try {
        HelloApp app{};
        app.run();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}
