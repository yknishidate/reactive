#include "Engine.hpp"

struct PushConstants
{
    glm::mat4 model{ 1 };
    glm::mat4 view{ 1 };
    glm::mat4 proj{ 1 };
};

std::string vertCode = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 proj;
} pushConstants;

void main() {
    gl_Position = pushConstants.proj * pushConstants.view * pushConstants.model * vec4(inPosition, 1.0);
    fragColor = vec3(1);
    fragNormal = inNormal;
})";

std::string fragCode = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0) * dot(fragNormal, vec3(1.0));
})";

int main()
{
    try {
        Log::init();
        Window::init(750, 750);
        Context::init();

        Swapchain swapchain{};
        GUI gui{ swapchain };

        //std::vector<Vertex> vertices{ {{-1, 0, 0}}, {{ 0, -1, 0}}, {{ 1, 0, 0}} };
        //std::vector<Index> indices{ 0, 1, 2 };
        //Mesh mesh{ vertices, indices };

        Mesh mesh{ "bunny.obj" };
        Camera camera{ Window::getWidth(), Window::getHeight() };

        Shader vertShader{ vertCode, vk::ShaderStageFlagBits::eVertex };
        Shader fragShader{ fragCode, vk::ShaderStageFlagBits::eFragment };

        DescriptorSet descSet;
        descSet.addResources(vertShader);
        descSet.addResources(fragShader);
        descSet.allocate();

        GraphicsPipeline pipeline{ descSet };
        pipeline.setVertexShader(vertShader);
        pipeline.setFragmentShader(fragShader);
        pipeline.setup(swapchain, sizeof(PushConstants));

        int testInt = 0;
        int frame = 0;
        while (!Window::shouldClose()) {
            Window::pollEvents();
            camera.processInput();

            PushConstants pushConstants;
            pushConstants.model = glm::rotate(glm::mat4(1.0f), 0.01f * frame, glm::vec3(0.0f, 1.0f, 0.0f));
            pushConstants.proj = camera.getProj();
            pushConstants.view = camera.getView();

            gui.startFrame();
            gui.sliderInt("Test slider", testInt, 0, 100);

            swapchain.waitNextFrame();

            CommandBuffer commandBuffer = swapchain.beginCommandBuffer();
            commandBuffer.bindPipeline(pipeline);
            commandBuffer.pushConstants(pipeline, &pushConstants);
            commandBuffer.clearBackImage({ 0.0f, 0.0f, 0.3f, 1.0f });
            commandBuffer.beginRenderPass();
            commandBuffer.drawIndexed(mesh);
            commandBuffer.drawGUI(gui);
            commandBuffer.endRenderPass();
            commandBuffer.submit();

            swapchain.present();
            frame++;
        }
        Context::waitIdle();
        Window::shutdown();
    } catch (const std::exception& e) {
        Log::error(e.what());
    }
}
