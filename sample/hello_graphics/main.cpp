#include <reactive/reactive.hpp>

using namespace rv;

std::string vertCode = R"(
#version 450
layout(location = 0) out vec4 outColor;
vec3 positions[] = vec3[](vec3(-1, -1, 0), vec3(0, 1, 0), vec3(1, -1, 0));
vec3 colors[] = vec3[](vec3(0), vec3(1, 0, 0), vec3(0, 1, 0));
void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 1);
    outColor = vec4(colors[gl_VertexIndex], 1);
})";

std::string fragCode = R"(
#version 450
layout(location = 0) in vec4 inColor;
layout(location = 0) out vec4 outColor;
void main() {
    outColor = inColor;
})";

class HelloApp : public App {
public:
    HelloApp()
        : App({
              .width = 1280,
              .height = 720,
              .title = "HelloGraphics",
              .vsync = false,
              .layers = {Layer::Validation, Layer::FPSMonitor},
          }) {}

    void onStart() override {
        std::vector<ShaderHandle> shaders(2);
        shaders[0] = context.createShader({
            .code = Compiler::compileToSPV(vertCode, vk::ShaderStageFlagBits::eVertex),
            .stage = vk::ShaderStageFlagBits::eVertex,
        });

        shaders[1] = context.createShader({
            .code = Compiler::compileToSPV(fragCode, vk::ShaderStageFlagBits::eFragment),
            .stage = vk::ShaderStageFlagBits::eFragment,
        });

        descSet = context.createDescriptorSet({
            .shaders = shaders,
        });

        pipeline = context.createGraphicsPipeline({
            .descSetLayout = descSet->getLayout(),
            .vertexShader = shaders[0],
            .fragmentShader = shaders[1],
        });

        gpuTimer = context.createGPUTimer({});
    }

    void onRender(const CommandBufferHandle& commandBuffer) override {
        if (frame > 0) {
            for (int i = 0; i < TIME_BUFFER_SIZE - 1; i++) {
                times[i] = times[i + 1];
            }
            float time = gpuTimer->elapsedInMilli();
            times[TIME_BUFFER_SIZE - 1] = time;
            ImGui::Text("GPU timer: %.3f ms", time);
            ImGui::PlotLines("Times", times, TIME_BUFFER_SIZE, 0, nullptr, FLT_MAX, FLT_MAX,
                             {300, 150});
        }

        commandBuffer->clearColorImage(getCurrentColorImage(), {0.0f, 0.0f, 0.5f, 1.0f});
        commandBuffer->setViewport(Window::getWidth(), Window::getHeight());
        commandBuffer->setScissor(Window::getWidth(), Window::getHeight());
        commandBuffer->bindDescriptorSet(descSet, pipeline);
        commandBuffer->bindPipeline(pipeline);
        commandBuffer->beginTimestamp(gpuTimer);
        commandBuffer->beginRendering(getCurrentColorImage(), nullptr, {0, 0},
                                      {Window::getWidth(), Window::getHeight()});
        commandBuffer->draw(3, 1, 0, 0);
        commandBuffer->endRendering();
        commandBuffer->endTimestamp(gpuTimer);
        frame++;
    }

    static constexpr int TIME_BUFFER_SIZE = 300;
    float times[TIME_BUFFER_SIZE] = {0};
    DescriptorSetHandle descSet;
    GraphicsPipelineHandle pipeline;
    GPUTimerHandle gpuTimer;
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
