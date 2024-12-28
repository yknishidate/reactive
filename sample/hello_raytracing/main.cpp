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
        camera = Camera{Camera::Type::Orbital,
                        static_cast<float>(Window::getWidth()) / Window::getHeight()};

        mesh = Mesh{context, MeshUsage::RayTracing,
                    MemoryUsage::Device, vertices, indices, "Triangle"};

        bottomAccel = context.createBottomAccel({
            .vertexStride = sizeof(Vertex),
            .maxVertexCount = mesh.getVertexCount(),
            .maxTriangleCount = mesh.getTriangleCount(),
            .debugName = "bottomAccel",
        });
        context.oneTimeSubmit([&](CommandBufferHandle commandBuffer) {
            commandBuffer->buildBottomAccel(bottomAccel, mesh.vertexBuffer, mesh.indexBuffer,
                                            mesh.getVertexCount(), mesh.getTriangleCount());
        });

        topAccel = context.createTopAccel({
            .accelInstances = {{bottomAccel}},
            .debugName = "topAccel",
        });

        image = context.createImage({
            .usage = ImageUsage::Storage,
            .extent = {Window::getWidth(), Window::getHeight(), 1},
            .format = vk::Format::eB8G8R8A8Unorm,
            .viewInfo = rv::ImageViewCreateInfo{},
        });

        context.oneTimeSubmit([&](CommandBufferHandle commandBuffer) {
            commandBuffer->buildTopAccel(topAccel);
            commandBuffer->transitionLayout(image, vk::ImageLayout::eGeneral);
        });

        
        SlangCompiler compiler;
        auto codes = compiler.CompileShaders(SHADER_DIR + "shaders.slang",
                                             {"rayGenMain", "missMain", "closestHitMain"});

        std::vector<ShaderHandle> shaders(3);
        shaders[0] = context.createShader({
            .pCode = codes[0]->getBufferPointer(),
            .codeSize = codes[0]->getBufferSize(),
            .stage = vk::ShaderStageFlagBits::eRaygenKHR,
        });

        shaders[1] = context.createShader({
            .pCode = codes[1]->getBufferPointer(),
            .codeSize = codes[1]->getBufferSize(),
            .stage = vk::ShaderStageFlagBits::eMissKHR,
        });

        shaders[2] = context.createShader({
            .pCode = codes[2]->getBufferPointer(),
            .codeSize = codes[2]->getBufferSize(),
            .stage = vk::ShaderStageFlagBits::eClosestHitKHR,
        });

        descSet = context.createDescriptorSet({
            .shaders = shaders,
            .images = {{"gOutputImage", image}},
            .accels = {{"gTopLevelAS", topAccel}},
        });
        descSet->update();

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

    void onUpdate(float dt) override {
        camera.processMouseDragLeft(Window::getMouseDragLeft());
        camera.processMouseScroll(Window::getMouseScroll());

        pushConstants.invProj = camera.getInvProj();
        pushConstants.invView = camera.getInvView();
    }

    void onRender(const CommandBufferHandle& commandBuffer) override {
        ImGui::SliderInt("Test slider", &testInt, 0, 100);

        commandBuffer->bindDescriptorSet(pipeline, descSet);
        commandBuffer->bindPipeline(pipeline);
        commandBuffer->pushConstants(pipeline, &pushConstants);
        commandBuffer->traceRays(pipeline, Window::getWidth(), Window::getHeight(), 1);
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

    Camera camera;
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
