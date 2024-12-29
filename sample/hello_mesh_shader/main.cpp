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
        auto codes = compiler.CompileShaders(SHADER_PATH, {"meshMain", "fragmentMain"});

        std::vector<ShaderHandle> shaders(2);
        shaders[0] = context.createShader({
            .pCode = codes[0]->getBufferPointer(),
            .codeSize = codes[0]->getBufferSize(),
            .stage = vk::ShaderStageFlagBits::eMeshEXT,
        });

        shaders[1] = context.createShader({
            .pCode = codes[1]->getBufferPointer(),
            .codeSize = codes[1]->getBufferSize(),
            .stage = vk::ShaderStageFlagBits::eFragment,
        });

        descSet = context.createDescriptorSet({
            .shaders = shaders,
        });

        pipeline = context.createMeshShaderPipeline({
            .descSetLayout = descSet->getLayout(),
            .taskShader = {},
            .meshShader = shaders[0],
            .fragmentShader = shaders[1],
        });
    }

    void onRender(const CommandBufferHandle& commandBuffer) override {
        ImGui::SliderInt("Test slider", &testInt, 0, 100);
        commandBuffer->clearColorImage(getCurrentColorImage(), {0.0f, 0.0f, 0.5f, 1.0f});
        commandBuffer->setViewport(Window::getWidth(), Window::getHeight());
        commandBuffer->setScissor(Window::getWidth(), Window::getHeight());
        commandBuffer->bindDescriptorSet(pipeline, descSet);
        commandBuffer->bindPipeline(pipeline);
        commandBuffer->beginRendering(getCurrentColorImage(), nullptr, {0, 0},
                                      {Window::getWidth(), Window::getHeight()});
        commandBuffer->drawMeshTasks(1, 1, 1);
        commandBuffer->endRendering();
    }

    DescriptorSetHandle descSet;
    MeshShaderPipelineHandle pipeline;
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
