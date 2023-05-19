#include "App.hpp"

std::string meshCode = R"(
#version 450
#extension GL_EXT_mesh_shader : require

layout(local_size_x = 4, local_size_y = 1, local_size_z = 1) in;
layout(triangles, max_vertices = 4, max_primitives = 2) out;

layout(location = 0) out VertexOutput
{
    vec4 color;
} vertexOutput[];

vec2 positions[] = vec2[](
    vec2(-0.5, -0.5),
    vec2( 0.5, -0.5),
    vec2( 0.5,  0.5),
    vec2(-0.5,  0.5)
);

uvec3 indices[] = uvec3[](
    uvec3(0, 1, 2),
    uvec3(2, 3, 0)
);

void main()
{
    uint gid = gl_GlobalInvocationID.x;
    SetMeshOutputsEXT(4, 2);
    gl_MeshVerticesEXT[gid].gl_Position.xy = positions[gid];
    vertexOutput[gid].color.xy = positions[gid] + 0.5;
    if(gid < 2){
        gl_PrimitiveTriangleIndicesEXT[gid] = indices[gid];
    }
})";

std::string fragCode = R"(
#version 450
layout (location = 0) in VertexInput {
    vec4 color;
} vertexInput;
layout(location = 0) out vec4 outColor;
void main() {
    outColor = vertexInput.color;
})";

class HelloApp : public App {
public:
    HelloApp()
        : App({
              .width = 1280,
              .height = 720,
              .title = "HelloGraphics",
              .enableMeshShader = true,
          }) {}

    void onStart() override {
        Shader meshShader = context.createShader({
            .glslCode = meshCode,
            .stage = vk::ShaderStageFlagBits::eMeshEXT,
        });

        Shader fragShader = context.createShader({
            .glslCode = fragCode,
            .stage = vk::ShaderStageFlagBits::eFragment,
        });

        descSet = context.createDescriptorSet({
            .shaders = {&meshShader, &fragShader},
        });

        pipeline = context.createMeshShaderPipeline({
            .renderPass = getDefaultRenderPass(),
            .descSetLayout = descSet.getLayout(),
            .task = {.shader = {}},
            .mesh = {.shader = meshShader},
            .fragment = {.shader = fragShader},
            .viewport = "dynamic",
            .scissor = "dynamic",
        });
    }

    void onRender(const CommandBuffer& commandBuffer) override {
        ImGui::SliderInt("Test slider", &testInt, 0, 100);
        commandBuffer.clearColorImage(getCurrentColorImage(), {0.0f, 0.0f, 0.5f, 1.0f});
        commandBuffer.setViewport(width, height);
        commandBuffer.setScissor(width, height);
        commandBuffer.bindDescriptorSet(descSet, pipeline);
        commandBuffer.bindPipeline(pipeline);
        commandBuffer.beginRenderPass(getDefaultRenderPass(), getCurrentFramebuffer(), width,
                                      height);
        commandBuffer.drawMeshTasks(1, 1, 1);
        commandBuffer.endRenderPass();
    }

    DescriptorSet descSet;
    MeshShaderPipeline pipeline;
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
