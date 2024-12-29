#include <reactive/reactive.hpp>

using namespace rv;

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
        SlangCompiler compiler;
        auto codes = compiler.compileShaders(SHADER_PATH, {"vertexMain", "fragmentMain"});

        std::vector<ShaderHandle> shaders(2);
        shaders[0] = m_context.createShader({
            .pCode = codes[0]->getBufferPointer(),
            .codeSize = codes[0]->getBufferSize(),
            .stage = vk::ShaderStageFlagBits::eVertex,
        });

        shaders[1] = m_context.createShader({
            .pCode = codes[1]->getBufferPointer(),
            .codeSize = codes[1]->getBufferSize(),
            .stage = vk::ShaderStageFlagBits::eFragment,
        });

        m_descSet = m_context.createDescriptorSet({
            .shaders = shaders,
        });

        m_pipeline = m_context.createGraphicsPipeline({
            .descSetLayout = m_descSet->getLayout(),
            .vertexShader = shaders[0],
            .fragmentShader = shaders[1],
        });

        m_gpuTimer = m_context.createGPUTimer({});
    }

    void onRender(const CommandBufferHandle& commandBuffer) override {
        if (m_frame > 0) {
            for (int i = 0; i < TIME_BUFFER_SIZE - 1; i++) {
                m_times[i] = m_times[i + 1];
            }
            float time = m_gpuTimer->elapsedInMilli();
            m_times[TIME_BUFFER_SIZE - 1] = time;
            ImGui::Text("GPU timer: %.3f ms", time);
            ImGui::PlotLines("Times", m_times, TIME_BUFFER_SIZE, 0, nullptr, FLT_MAX, FLT_MAX,
                             {300, 150});
        }

        commandBuffer->clearColorImage(getCurrentColorImage(), {0.0f, 0.0f, 0.5f, 1.0f});
        commandBuffer->setViewport(Window::getWidth(), Window::getHeight());
        commandBuffer->setScissor(Window::getWidth(), Window::getHeight());
        commandBuffer->bindDescriptorSet(m_pipeline, m_descSet);
        commandBuffer->bindPipeline(m_pipeline);
        commandBuffer->beginTimestamp(m_gpuTimer);
        commandBuffer->beginRendering(getCurrentColorImage(), nullptr, {0, 0},
                                      {Window::getWidth(), Window::getHeight()});
        commandBuffer->draw(3, 1, 0, 0);
        commandBuffer->endRendering();
        commandBuffer->endTimestamp(m_gpuTimer);
        m_frame++;
    }

    static constexpr int TIME_BUFFER_SIZE = 300;
    float m_times[TIME_BUFFER_SIZE] = {0};
    DescriptorSetHandle m_descSet;
    GraphicsPipelineHandle m_pipeline;
    GPUTimerHandle m_gpuTimer;
    int m_frame = 0;
};

int main() {
    try {
        HelloApp app{};
        app.run();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}
