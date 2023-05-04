#include "App.hpp"

std::string vertCode = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable
vec3 positions[] = vec3[](vec3(-0.5, 0.5, 0), vec3(0, -0.5, 0), vec3(0.5, 0.5, 0));
void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 1);
})";

std::string fragCode = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 outColor;
void main() {
    outColor = vec4(1.0);
})";

class HelloApp : public App {
public:
    HelloApp()
        : App({
              .windowWidth = 1280,
              .windowHeight = 720,
              .title = "HelloGraphics",
          }) {}

    void onStart() override {
        vertShader = context.createShader({
            .glslCode = vertCode,
            .shaderStage = vk::ShaderStageFlagBits::eVertex,
        });

        fragShader = context.createShader({
            .glslCode = fragCode,
            .shaderStage = vk::ShaderStageFlagBits::eFragment,
        });

        descSet = context.createDescriptorSet({
            .shaders = {&vertShader, &fragShader},
        });

        pipeline = context.createGraphicsPipeline({
            .vertexShader = &vertShader,
            .fragmentShader = &fragShader,
            .renderPass = getDefaultRenderPass(),
            .descSetLayout = descSet.getLayout(),
            .width = width,
            .height = height,
        });
    }

    void onRender(const CommandBuffer& commandBuffer) override {
        ImGui::SliderInt("Test slider", &testInt, 0, 100);
        commandBuffer.clearColorImage(getBackImage(), {0.0f, 0.0f, 0.5f, 1.0f});
        commandBuffer.bindPipeline(pipeline);
        commandBuffer.beginRenderPass(getDefaultRenderPass(), getBackFramebuffer(), width, height);
        commandBuffer.draw(3, 1, 0, 0);
        commandBuffer.endRenderPass();
    }

    Shader vertShader;
    Shader fragShader;
    DescriptorSet descSet;
    GraphicsPipeline pipeline;
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
