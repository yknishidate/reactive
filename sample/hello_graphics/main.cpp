// #include "Engine.hpp"
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

// int main() {
//     try {
//         Window::init(750, 750);
//         Context::init(true);
//
//         Swapchain swapchain{};
//         GUI::init(swapchain);
//
//         std::vector<Vertex> vertices{{{-1, 0, 0}}, {{0, -1, 0}}, {{1, 0, 0}}};
//         std::vector<Index> indices{0, 1, 2};
//         Mesh mesh{vertices, indices};
//
//         Camera camera{Window::getWidth(), Window::getHeight()};
//
//         Shader vertShader{vertCode, vk::ShaderStageFlagBits::eVertex};
//         Shader fragShader{fragCode, vk::ShaderStageFlagBits::eFragment};
//
//         DescriptorSet descSet;
//         descSet.addResources(vertShader);
//         descSet.addResources(fragShader);
//         descSet.allocate();
//
//         GraphicsPipeline pipeline;
//         pipeline.setDescriptorSet(descSet);
//         pipeline.addShader(vertShader);
//         pipeline.addShader(fragShader);
//         pipeline.setPushSize(sizeof(PushConstants));
//         pipeline.setup(swapchain.getRenderPass());
//
//         int testInt = 0;
//         int frame = 0;
//         while (!Window::shouldClose()) {
//             Window::pollEvents();
//             camera.processInput();
//
//             PushConstants pushConstants;
//             pushConstants.model = glm::rotate(glm::mat4(1), 0.01f * frame, glm::vec3(0, 1, 0));
//             pushConstants.proj = camera.getProj();
//             pushConstants.view = camera.getView();
//
//             GUI::startFrame();
//             ImGui::SliderInt("Test slider", &testInt, 0, 100);
//
//             swapchain.waitNextFrame();
//
//             CommandBuffer commandBuffer = swapchain.beginCommandBuffer();
//             commandBuffer.bindPipeline(pipeline);
//             commandBuffer.pushConstants(pipeline, &pushConstants);
//             commandBuffer.clearBackImage({0.0f, 0.0f, 0.3f, 1.0f});
//             commandBuffer.beginDefaultRenderPass();
//             commandBuffer.drawIndexed(mesh);
//             GUI::render(commandBuffer.commandBuffer);
//             commandBuffer.endDefaultRenderPass();
//             commandBuffer.submit();
//
//             swapchain.present();
//             frame++;
//         }
//         Context::waitIdle();
//         Window::shutdown();
//         GUI::shutdown();
//     } catch (const std::exception& e) {
//         Log::error(e.what());
//     }
// }

#include "App.hpp"
#include "Engine.hpp"

#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

struct Camera {
    Camera() = default;
    Camera(uint32_t width, uint32_t height) { aspect = static_cast<float>(width) / height; }

    // void processInput() {
    //     if (Window::mousePressed()) {
    //         glm::vec2 motion = Window::getMouseMotion();
    //         yaw = glm::mod(yaw - motion.x * 0.1f, 360.0f);
    //         pitch = glm::clamp(pitch + motion.y * 0.1f, -89.9f, 89.9f);
    //         updateFront();
    //         dirty = true;
    //     }

    //    glm::vec3 forward = getFront();
    //    glm::vec3 right = getRight();
    //    if (Window::keyPressed(Key::W)) {
    //        position += forward * 0.15f * speed;
    //        dirty = true;
    //    }
    //    if (Window::keyPressed(Key::S)) {
    //        position -= forward * 0.15f * speed;
    //        dirty = true;
    //    }
    //    if (Window::keyPressed(Key::D)) {
    //        position += right * 0.1f * speed;
    //        dirty = true;
    //    }
    //    if (Window::keyPressed(Key::A)) {
    //        position -= right * 0.1f * speed;
    //        dirty = true;
    //    }
    //    if (Window::keyPressed(Key::Space)) {
    //        position.y -= 0.05f * speed;
    //        dirty = true;
    //    }
    //}

    glm::vec3 getFront() const { return glm::normalize(front); }
    glm::vec3 getUp() const { return glm::vec3{0, 1, 0}; }
    glm::vec3 getRight() const { return glm::normalize(glm::cross(-getUp(), getFront())); }
    glm::vec3 getPosition() const { return position; }
    glm::mat4 getView() const { return glm::lookAt(position, position + front, getUp()); }
    glm::mat4 getProj() const { return glm::perspective(getFovY(), aspect, zNear, zFar); }
    glm::mat4 getInvView() const { return glm::inverse(getView()); }
    glm::mat4 getInvProj() const { return glm::inverse(getProj()); }
    float getFovY() const { return fovY; }
    float getAspect() const { return aspect; }
    float getNear() const { return zNear; }
    float getFar() const { return zFar; }
    float getPitch() const { return pitch; }
    float getYaw() const { return yaw; }

    void setPosition(float x, float y, float z) { position = {x, y, z}; }
    void setPosition(glm::vec3 pos) { position = pos; }
    void setSpeed(float s) { speed = s; }
    void setPitch(float p) {
        pitch = p;
        updateFront();
    }
    void setYaw(float y) {
        yaw = y;
        updateFront();
    }
    void setNear(float n) { zNear = n; }
    void setFar(float f) { zFar = f; }
    void setFovY(float f) { fovY = f; }

    void updateFront() {
        glm::mat4 rotation{1.0};
        rotation *= glm::rotate(glm::radians(yaw), glm::vec3{0, 1, 0});
        rotation *= glm::rotate(glm::radians(pitch), glm::vec3{1, 0, 0});
        front = {rotation * glm::vec4{0, 0, -1, 1}};
    }
    float speed = 1.0f;
    glm::vec3 position = {0.0f, 0.0f, 5.0f};
    glm::vec3 front = {0.0f, 0.0f, -1.0f};
    float aspect = 1.0f;
    float zNear = 0.01f;
    float zFar = 10000.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    float fovY = glm::radians(45.0f);
};

class HelloApp : public App {
public:
    HelloApp() : App(1280, 720, "Hello", true) {}

    void onStart() override {
        std::vector<Vertex> vertices{{{-1, 0, 0}}, {{0, -1, 0}}, {{1, 0, 0}}};
        std::vector<Index> indices{0, 1, 2};
        vertexBuffer = DeviceBuffer{this, BufferUsage::Vertex, vertices};
        indexBuffer = DeviceBuffer{this, BufferUsage::Index, indices};
        spdlog::info("Buffers are created.");

        camera = Camera{static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height)};

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

    DeviceBuffer vertexBuffer;
    DeviceBuffer indexBuffer;
    Camera camera;
    Shader vertShader;
    Shader fragShader;
    DescriptorSet descSet;
    GraphicsPipeline pipeline;
};

int main() {
    HelloApp app{};
    app.run();
}
