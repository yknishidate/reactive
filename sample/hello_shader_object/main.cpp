#include "App.hpp"

std::string vertCode = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 outColor;
vec3 positions[] = vec3[](vec3(-1, 1, 0), vec3(0, -1, 0), vec3(1, 1, 0));
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
              .title = "HelloShaderObject",
              .enableShaderObject = true,
          }) {}

    ~HelloApp() {
        for (auto& shader : shaders) {
            context.getDevice().destroyShaderEXT(shader);
        }
    }

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

        vk::DescriptorSetLayout setLayout = descSet.getLayout();
        std::vector<uint32_t> vertSpvCode = vertShader.getSpvCode();
        std::vector<uint32_t> fragSpvCode = fragShader.getSpvCode();

        vk::ShaderCreateInfoEXT shaderInfo[2] = {
            vk::ShaderCreateInfoEXT()
                .setFlags(vk::ShaderCreateFlagBitsEXT::eLinkStage)
                .setStage(vk::ShaderStageFlagBits::eVertex)
                .setNextStage(vk::ShaderStageFlagBits::eFragment)
                .setCodeType(vk::ShaderCodeTypeEXT::eSpirv)
                .setCodeSize(vertSpvCode.size() * sizeof(uint32_t))
                .setPCode(vertSpvCode.data())
                .setPName("main")
                .setSetLayouts(setLayout),
            vk::ShaderCreateInfoEXT()
                .setFlags(vk::ShaderCreateFlagBitsEXT::eLinkStage)
                .setStage(vk::ShaderStageFlagBits::eFragment)
                .setCodeType(vk::ShaderCodeTypeEXT::eSpirv)
                .setCodeSize(fragSpvCode.size() * sizeof(uint32_t))
                .setPCode(fragSpvCode.data())
                .setPName("main")
                .setSetLayouts(setLayout)};

        shaders = context.getDevice().createShadersEXT(shaderInfo);
    }

    void onRender(const CommandBuffer& commandBuffer) override {
        std::vector<vk::ShaderStageFlagBits> stages = {
            vk::ShaderStageFlagBits::eVertex,
            vk::ShaderStageFlagBits::eFragment,
        };

        commandBuffer.clearColorImage(getCurrentColorImage(), {0.0f, 0.0f, 0.5f, 1.0f});
        commandBuffer.setViewport(width, height);
        commandBuffer.setScissor(width, height);
        commandBuffer.commandBuffer.bindShadersEXT(stages, shaders);
        commandBuffer.beginRenderPass(getDefaultRenderPass(), getCurrentFramebuffer(), width,
                                      height);
        commandBuffer.draw(3, 1, 0, 0);
        commandBuffer.endRenderPass();
    }

    DescriptorSet descSet;
    std::vector<vk::ShaderEXT> shaders;
};

int main() {
    try {
        HelloApp app{};
        app.run();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}
