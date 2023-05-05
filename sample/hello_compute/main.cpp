#include "App.hpp"

class HelloApp : public App {
public:
    HelloApp()
        : App({
              .width = 1280,
              .height = 720,
              .title = "HelloCompute",
          }) {}

    void onStart() override {
        image = context.createImage({
            .usage = ImageUsage::GeneralStorage,
            .width = width,
            .height = height,
        });

        Shader compShader = context.createShader({
            .glslCode = File::readFile(SHADER_DIR + "hello_compute.comp"),
            .stage = vk::ShaderStageFlagBits::eCompute,
        });

        descSet = context.createDescriptorSet({
            .shaders = {&compShader},
            .images = {{"outputImage", image}},
        });

        pipeline = context.createComputePipeline({
            .computeShader = &compShader,
            .descSetLayout = descSet.getLayout(),
            .pushSize = sizeof(int),
        });
    }

    void onUpdate() override { frame++; }

    void onRender(const CommandBuffer& commandBuffer) override {
        ImGui::SliderInt("Test slider", &testInt, 0, 100);
        commandBuffer.bindDescriptorSet(descSet, pipeline);
        commandBuffer.bindPipeline(pipeline);
        commandBuffer.pushConstants(pipeline, &frame);
        commandBuffer.dispatch(pipeline, width, height, 1);
        commandBuffer.copyImage(image.getImage(), getCurrentImage(), vk::ImageLayout::eGeneral,
                                vk::ImageLayout::ePresentSrcKHR, width, height);
    }

    Image image;
    DescriptorSet descSet;
    ComputePipeline pipeline;
    int testInt = 0;
    int frame = 0;
};

int main() {
    try {
        HelloApp app{};
        app.run();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}
