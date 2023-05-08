#include "App.hpp"

std::string vertCode = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 outColor;
vec3 positions[] = vec3[](vec3(-0.5, 0.5, 0), vec3(0, -0.5, 0), vec3(0.5, 0.5, 0));
vec3 colors[] = vec3[](vec3(0), vec3(1, 0, 0), vec3(0, 1, 0));
void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 1);
    outColor = vec4(colors[gl_VertexIndex], 1);
})";

std::string fragCode = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable
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
          }) {}

    void onStart() override {
        Shader vertShader = context.createShader({
            .glslCode = vertCode,
            .stage = vk::ShaderStageFlagBits::eVertex,
        });

        Shader fragShader = context.createShader({
            .glslCode = fragCode,
            .stage = vk::ShaderStageFlagBits::eFragment,
        });

        descSet = context.createDescriptorSet({
            .shaders = {&vertShader, &fragShader},
        });

        pipeline = context.createGraphicsPipeline({
            .renderPass = getDefaultRenderPass(),
            .descSetLayout = descSet.getLayout(),
            .vertex = {.shader = vertShader},
            .fragment = {.shader = fragShader},
            .viewport = {.width = width, .height = height},
        });
    }

    void onRender(const CommandBuffer& commandBuffer) override {
        ImGui::SliderInt("Test slider", &testInt, 0, 100);
        commandBuffer.clearColorImage(getCurrentImage(), {0.0f, 0.0f, 0.5f, 1.0f});
        commandBuffer.bindPipeline(pipeline);
        commandBuffer.beginRenderPass(getDefaultRenderPass(), getCurrentFramebuffer(), width,
                                      height);
        commandBuffer.draw(3, 1, 0, 0);
        commandBuffer.endRenderPass();
    }

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
