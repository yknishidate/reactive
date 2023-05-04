#include "App.hpp"

struct PushConstants {
    glm::mat4 invView{1};
    glm::mat4 invProj{1};
};

class HelloApp : public App {
public:
    HelloApp()
        : App({
              .windowWidth = 1280,
              .windowHeight = 720,
              .title = "HelloRayTracing",
              .enableRayTracing = true,
          }) {}

    void onStart() override {
        camera = OrbitalCamera{this, width, height};

        mesh = context.createMesh({
            .vertices = vertices,
            .indices = indices,
        });

        bottomAccel = context.createBottomAccel({
            .mesh = &mesh,
        });

        topAccel = context.createTopAccel({
            .bottomAccels = {{&bottomAccel, glm::mat4{1.0}}},
        });

        image = context.createImage({
            .usage = ImageUsage::GeneralStorage,
            .width = width,
            .height = height,
        });

        std::string rgenCode = File::readFile(SHADER_DIR + "hello_raytracing.rgen");
        std::string missCode = File::readFile(SHADER_DIR + "hello_raytracing.rmiss");
        std::string chitCode = File::readFile(SHADER_DIR + "hello_raytracing.rchit");

        rgenShader = context.createShader({
            .glslCode = rgenCode,
            .shaderStage = vk::ShaderStageFlagBits::eRaygenKHR,
        });

        missShader = context.createShader({
            .glslCode = missCode,
            .shaderStage = vk::ShaderStageFlagBits::eMissKHR,
        });

        chitShader = context.createShader({
            .glslCode = chitCode,
            .shaderStage = vk::ShaderStageFlagBits::eClosestHitKHR,
        });

        descSet = context.createDescriptorSet({
            .shaders = {&rgenShader, &missShader, &chitShader},
            .images = {{"outputImage", image}},
            .accels = {{"topLevelAS", topAccel}},
        });

        pipeline = context.createRayTracingPipeline({
            .rgenShader = &rgenShader,
            .missShader = &missShader,
            .chitShader = &chitShader,
            .descSetLayout = descSet.getLayout(),
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
        commandBuffer.copyImageToImage(image.getImage(), getBackImage(),  // images
                                       vk::ImageLayout::eGeneral, vk::ImageLayout::ePresentSrcKHR,
                                       width, height);
    }

    std::vector<Vertex> vertices{{{-1, 0, 0}}, {{0, -1, 0}}, {{1, 0, 0}}};
    std::vector<uint32_t> indices{0, 1, 2};
    Mesh mesh;
    BottomAccel bottomAccel;
    TopAccel topAccel;
    Image image;

    Shader rgenShader;
    Shader missShader;
    Shader chitShader;
    DescriptorSet descSet;
    RayTracingPipeline pipeline;

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
