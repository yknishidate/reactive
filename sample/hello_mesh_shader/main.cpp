#include <reactive/reactive.hpp>

using namespace rv;

class HelloApp : public App {
public:
    HelloApp()
        : App({
              .width = 1280,
              .height = 720,
              .title = "HelloMeshShader",
              .layers = {Layer::Validation},
              .extensions = {Extension::MeshShader},
          }) {}

    void onStart() override {
        SlangCompiler compiler;
        auto codes = compiler.compileShaders(SHADER_PATH, {"meshMain", "fragmentMain"});

        std::vector<ShaderHandle> shaders(2);
        shaders[0] = m_context.createShader({
            .pCode = codes[0]->getBufferPointer(),
            .codeSize = codes[0]->getBufferSize(),
            .stage = vk::ShaderStageFlagBits::eMeshEXT,
        });

        shaders[1] = m_context.createShader({
            .pCode = codes[1]->getBufferPointer(),
            .codeSize = codes[1]->getBufferSize(),
            .stage = vk::ShaderStageFlagBits::eFragment,
        });

        m_descSet = m_context.createDescriptorSet({
            .shaders = shaders,
        });

        m_pipeline = m_context.createMeshShaderPipeline({
            .descSetLayout = m_descSet->getLayout(),
            .taskShader = {},
            .meshShader = shaders[0],
            .fragmentShader = shaders[1],
        });
    }

    void onRender(const CommandBufferHandle& commandBuffer) override {
        ImGui::SliderInt("Test slider", &m_testInt, 0, 100);
        commandBuffer->clearColorImage(getCurrentColorImage(), {0.0f, 0.0f, 0.5f, 1.0f});
        commandBuffer->setViewport(Window::getWidth(), Window::getHeight());
        commandBuffer->setScissor(Window::getWidth(), Window::getHeight());
        commandBuffer->bindDescriptorSet(m_pipeline, m_descSet);
        commandBuffer->bindPipeline(m_pipeline);
        commandBuffer->beginRendering(getCurrentColorImage(), nullptr, {0, 0},
                                      {Window::getWidth(), Window::getHeight()});
        commandBuffer->drawMeshTasks(1, 1, 1);
        commandBuffer->endRendering();
    }

    DescriptorSetHandle m_descSet;
    MeshShaderPipelineHandle m_pipeline;
    int m_testInt = 0;
};

int main() {
    try {
        HelloApp app{};
        app.run();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}
