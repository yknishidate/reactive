#include <reactive/App.hpp>

using namespace rv;

class HelloApp : public App {
public:
    HelloApp()
        : App({
              .width = 1280,
              .height = 720,
              .title = "HelloCompute",
              .layers = {Layer::Validation},
          }) {}

    void onStart() override {
        image = context.createImage({
            .usage = ImageUsage::Storage,
            .extent = {width, height, 1},
            .format = vk::Format::eB8G8R8A8Unorm,
            .layout = vk::ImageLayout::eGeneral,
        });

        ShaderHandle compShader = context.createShader({
            .code = Compiler::compileToSPV(SHADER_DIR + "hello_compute.comp"),
            .stage = vk::ShaderStageFlagBits::eCompute,
        });

        descSet = context.createDescriptorSet({
            .shaders = compShader,
            .images = {{"outputImage", image}},
        });

        pipeline = context.createComputePipeline({
            .computeShader = compShader,
            .descSetLayout = descSet->getLayout(),
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
        commandBuffer.copyImage(image, getCurrentColorImage(), vk::ImageLayout::eGeneral,
                                vk::ImageLayout::ePresentSrcKHR);
    }

    ImageHandle image;
    DescriptorSetHandle descSet;
    ComputePipelineHandle pipeline;
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
