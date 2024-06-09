#include <reactive/reactive.hpp>

using namespace rv;

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
};
layout(location = 0) out vec4 outColor;

void main() {
    outColor = color;
})";

class HelloApp : public App {
public:
    HelloApp()
        : App({
              .width = 1280,
              .height = 720,
              .title = "HelloGraphics",
              .layers = {Layer::Validation},
              .extensions = {Extension::MeshShader},
          }) {}

    void onStart() override {
        std::vector<ShaderHandle> shaders(2);
        shaders[0] = context.createShader({
            .code = Compiler::compileToSPV(meshCode, vk::ShaderStageFlagBits::eMeshEXT),
            .stage = vk::ShaderStageFlagBits::eMeshEXT,
        });

        shaders[1] = context.createShader({
            .code = Compiler::compileToSPV(fragCode, vk::ShaderStageFlagBits::eFragment),
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
        commandBuffer->bindDescriptorSet(descSet, pipeline);
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
