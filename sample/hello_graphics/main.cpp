#include "App.hpp"
#include "Engine.hpp"

struct PushConstants {
    glm::mat4 model{1};
    glm::mat4 view{1};
    glm::mat4 proj{1};
};

std::string vertCode = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 proj;
} pushConstants;

void main() {
    gl_Position = pushConstants.proj * pushConstants.view * pushConstants.model *
    vec4(inPosition, 1.0);
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
    HelloApp() : App(1280, 720, "Hello", true) {}

    void onStart() override {
        vertexBuffer = DeviceBuffer{this, BufferUsage::Vertex, vertices};
        indexBuffer = DeviceBuffer{this, BufferUsage::Index, indices};
        spdlog::info("Buffers are created.");

        camera = Camera{this, static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height)};

        vertShader = Shader{this, vertCode, vk::ShaderStageFlagBits::eVertex};
        fragShader = Shader{this, fragCode, vk::ShaderStageFlagBits::eFragment};
        spdlog::info("Shaders are created.");

        descSet = DescriptorSet{this};
        descSet.addResources(vertShader);
        descSet.addResources(fragShader);
        descSet.allocate();
        spdlog::info("DescriptorSet is created.");

        pipeline = GraphicsPipeline{this};
        pipeline.setDescriptorSet(descSet);
        pipeline.addShader(vertShader);
        pipeline.addShader(fragShader);
        pipeline.setPushSize(sizeof(PushConstants));
        pipeline.setup(*renderPass);
        spdlog::info("Pipeline is created.");
    }

    void onUpdate() override {
        camera.processInput();

        pushConstants.model = glm::rotate(glm::mat4(1), 0.01f * frame, glm::vec3(0, 1, 0));
        pushConstants.proj = camera.getProj();
        pushConstants.view = camera.getView();
    }

    void onRender(const CommandBuffer& commandBuffer) override {
        ImGui::SliderInt("Test slider", &testInt, 0, 100);

        commandBuffer.clearBackImage({0.0f, 0.0f, 0.5f, 1.0f});
        commandBuffer.bindPipeline(pipeline);
        commandBuffer.pushConstants(pipeline, &pushConstants);
        commandBuffer.beginDefaultRenderPass();
        commandBuffer.drawIndexed(vertexBuffer, indexBuffer, indices.size());
        commandBuffer.endRenderPass();

        frame++;
    }

    std::vector<Vertex> vertices{{{-1, 0, 0}}, {{0, -1, 0}}, {{1, 0, 0}}};
    std::vector<Index> indices{0, 1, 2};
    DeviceBuffer vertexBuffer;
    DeviceBuffer indexBuffer;
    Camera camera;
    Shader vertShader;
    Shader fragShader;
    DescriptorSet descSet;
    GraphicsPipeline pipeline;

    PushConstants pushConstants;
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
