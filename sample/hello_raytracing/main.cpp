#include <reactive/reactive.hpp>

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

        mesh = Mesh{context, vertices, indices, "Triangle"};

        bottomAccel = context.createBottomAccel({
            .vertexBuffer = mesh.vertexBuffer,
            .indexBuffer = mesh.indexBuffer,
            .vertexStride = sizeof(Vertex),
            .vertexCount = mesh.getVertexCount(),
            .triangleCount = mesh.getTriangleCount(),
        });
        context.oneTimeSubmit([&](CommandBufferHandle commandBuffer) {
            commandBuffer->buildBottomAccel(bottomAccel);
        });

        topAccel = context.createTopAccel({
            .accelInstances = {{bottomAccel}},
        });

        image = context.createImage({
            .usage = ImageUsage::Storage,
            .extent = {width, height, 1},
            .format = vk::Format::eB8G8R8A8Unorm,
        });

        context.oneTimeSubmit([&](CommandBufferHandle commandBuffer) {
            commandBuffer->buildTopAccel(topAccel);
            commandBuffer->transitionLayout(image, vk::ImageLayout::eGeneral);
        });

        std::vector<ShaderHandle> shaders(3);
        shaders[0] = context.createShader({
            .code = Compiler::compileToSPV(SHADER_DIR + "hello_raytracing.rgen"),
            .stage = vk::ShaderStageFlagBits::eRaygenKHR,
        });

        shaders[1] = context.createShader({
            .code = Compiler::compileToSPV(SHADER_DIR + "hello_raytracing.rmiss"),
            .stage = vk::ShaderStageFlagBits::eMissKHR,
        });

        shaders[2] = context.createShader({
            .code = Compiler::compileToSPV(SHADER_DIR + "hello_raytracing.rchit"),
            .stage = vk::ShaderStageFlagBits::eClosestHitKHR,
        });

        descSet = context.createDescriptorSet({
            .shaders = shaders,
            .images = {{"outputImage", image}},
            .accels = {{"topLevelAS", topAccel}},
        });

        pipeline = context.createRayTracingPipeline({
            .rgenGroup = RaygenGroup{.raygenShader = shaders[0]},
            .missGroups = {MissGroup{.missShader = shaders[1]}},
            .hitGroups = {HitGroup{.chitShader = shaders[2]}},
            .callableGroups = {},
            .descSetLayout = descSet->getLayout(),
            .pushSize = sizeof(PushConstants),
            .maxRayRecursionDepth = 4,
        });
    }

    void onUpdate() override {
        camera.processInput();
        pushConstants.invProj = camera.getInvProj();
        pushConstants.invView = camera.getInvView();
    }

    void onRender(const CommandBufferHandle& commandBuffer) override {
        ImGui::SliderInt("Test slider", &testInt, 0, 100);

        commandBuffer->bindDescriptorSet(descSet, pipeline);
        commandBuffer->bindPipeline(pipeline);
        commandBuffer->pushConstants(pipeline, &pushConstants);
        commandBuffer->traceRays(pipeline, width, height, 1);
        commandBuffer->copyImage(image, getCurrentColorImage(), vk::ImageLayout::eGeneral,
                                 vk::ImageLayout::ePresentSrcKHR);
    }

    std::vector<Vertex> vertices{{{-1, 0, 0}}, {{0, -1, 0}}, {{1, 0, 0}}};
    std::vector<uint32_t> indices{0, 1, 2};
    Mesh mesh;

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
