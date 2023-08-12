#include <reactive/App.hpp>

using namespace rv;

struct PushConstants {
    glm::mat4 invView{1};
    glm::mat4 invProj{1};
};

class HelloApp : public App {
public:
    HelloApp()
        : App({
              .width = 1280,
              .height = 720,
              .title = "HelloRayTracing",
              .windowResizable = false,
              .layers = {Layer::Validation},
              .extensions = {Extension::RayTracing},
          }) {}

    void onStart() override {
        camera = OrbitalCamera{this, width, height};

        mesh = context.createMesh({
            .vertices = vertices,
            .indices = indices,
        });

        bottomAccel = context.createBottomAccel({
            .vertexBuffer = mesh->vertexBuffer,
            .indexBuffer = mesh->indexBuffer,
            .vertexStride = sizeof(Vertex),
            .vertexCount = static_cast<uint32_t>(mesh->vertices.size()),
            .triangleCount = mesh->getTriangleCount(),
        });

        topAccel = context.createTopAccel({.accelInstances = {{bottomAccel}}});

        image = context.createImage({
            .usage = ImageUsage::Storage,
            .width = width,
            .height = height,
            .format = Format::BGRA8Unorm,
            .layout = ImageLayout::General,
        });

        std::vector<ShaderHandle> shaders(3);
        shaders[0] = context.createShader({
            .code = Compiler::compileToSPV(SHADER_DIR + "hello_raytracing.rgen"),
            .stage = ShaderStage::Raygen,
        });

        shaders[1] = context.createShader({
            .code = Compiler::compileToSPV(SHADER_DIR + "hello_raytracing.rmiss"),
            .stage = ShaderStage::Miss,
        });

        shaders[2] = context.createShader({
            .code = Compiler::compileToSPV(SHADER_DIR + "hello_raytracing.rchit"),
            .stage = ShaderStage::ClosestHit,
        });

        descSet = context.createDescriptorSet({
            .shaders = shaders,
            .images = {{"outputImage", image}},
            .accels = {{"topLevelAS", topAccel}},
        });

        pipeline = context.createRayTracingPipeline({
            .rgenShaders = shaders[0],
            .missShaders = shaders[1],
            .chitShaders = shaders[2],
            .descSetLayout = descSet->getLayout(),
            .pushSize = sizeof(PushConstants),
        });
    }

    void onUpdate() override {
        camera.processInput();
        pushConstants.invProj = camera.getInvProj();
        pushConstants.invView = camera.getInvView();
    }

    void onRender(const CommandBuffer& commandBuffer) override {
        ImGui::SliderInt("Test slider", &testInt, 0, 100);

        commandBuffer.bindDescriptorSet(descSet, pipeline);
        commandBuffer.bindPipeline(pipeline);
        commandBuffer.pushConstants(pipeline, &pushConstants);
        commandBuffer.traceRays(pipeline, width, height, 1);
        commandBuffer.copyImage(image->getImage(), getCurrentColorImage()->getImage(),
                                vk::ImageLayout::eGeneral, vk::ImageLayout::ePresentSrcKHR, width,
                                height);
    }

    std::vector<Vertex> vertices{{{-1, 0, 0}}, {{0, -1, 0}}, {{1, 0, 0}}};
    std::vector<uint32_t> indices{0, 1, 2};
    MeshHandle mesh;
    BottomAccelHandle bottomAccel;
    TopAccelHandle topAccel;
    ImageHandle image;

    DescriptorSetHandle descSet;
    RayTracingPipelineHandle pipeline;

    OrbitalCamera camera;
    PushConstants pushConstants;
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
