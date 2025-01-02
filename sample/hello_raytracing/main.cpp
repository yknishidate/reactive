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
        m_camera = Camera{Camera::Type::Orbital,
                        static_cast<float>(Window::getWidth()) / Window::getHeight()};

        m_mesh = Mesh{m_context, MeshUsage::RayTracing,
                    MemoryUsage::Device, m_vertices, m_indices, "Triangle"};

        m_bottomAccel = m_context.createBottomAccel({
            .vertexStride = sizeof(Vertex),
            .maxVertexCount = m_mesh.getVertexCount(),
            .maxTriangleCount = m_mesh.getTriangleCount(),
            .debugName = "m_bottomAccel",
        });
        m_context.oneTimeSubmit([&](CommandBufferHandle commandBuffer) {
            commandBuffer->buildBottomAccel(m_bottomAccel,
                                            m_mesh.getVertexBuffer(), m_mesh.getIndexBuffer(),
                                            m_mesh.getVertexCount(), m_mesh.getTriangleCount());
        });

        m_topAccel = m_context.createTopAccel({
            .accelInstances = {{m_bottomAccel}},
            .debugName = "m_topAccel",
        });

        m_image = m_context.createImage({
            .usage = ImageUsage::Storage,
            .extent = {Window::getWidth(), Window::getHeight(), 1},
            .format = vk::Format::eB8G8R8A8Unorm,
            .viewInfo = rv::ImageViewCreateInfo{},
        });

        m_context.oneTimeSubmit([&](CommandBufferHandle commandBuffer) {
            commandBuffer->buildTopAccel(m_topAccel);
            commandBuffer->transitionLayout(m_image, vk::ImageLayout::eGeneral);
        });

        SlangCompiler compiler;
        auto codes = compiler.compileShaders(SHADER_PATH,
                                             {"rayGenMain", "missMain", "closestHitMain"});

        std::vector<ShaderHandle> shaders(3);
        shaders[0] = m_context.createShader({
            .pCode = codes[0]->getBufferPointer(),
            .codeSize = codes[0]->getBufferSize(),
            .stage = vk::ShaderStageFlagBits::eRaygenKHR,
        });

        shaders[1] = m_context.createShader({
            .pCode = codes[1]->getBufferPointer(),
            .codeSize = codes[1]->getBufferSize(),
            .stage = vk::ShaderStageFlagBits::eMissKHR,
        });

        shaders[2] = m_context.createShader({
            .pCode = codes[2]->getBufferPointer(),
            .codeSize = codes[2]->getBufferSize(),
            .stage = vk::ShaderStageFlagBits::eClosestHitKHR,
        });

        m_descSet = m_context.createDescriptorSet({
            .shaders = shaders,
            .images = {{"gOutputImage", m_image}},
            .accels = {{"gTopLevelAS", m_topAccel}},
        });
        m_descSet->update();

        m_pipeline = m_context.createRayTracingPipeline({
            .rgenGroup = RaygenGroup{.raygenShader = shaders[0]},
            .missGroups = {MissGroup{.missShader = shaders[1]}},
            .hitGroups = {HitGroup{.chitShader = shaders[2]}},
            .callableGroups = {},
            .descSetLayout = m_descSet->getLayout(),
            .pushSize = sizeof(PushConstants),
            .maxRayRecursionDepth = 4,
        });
    }

    void onUpdate(float dt) override {
        m_camera.processMouseDragLeft(Window::getMouseDragLeft());
        m_camera.processMouseScroll(Window::getMouseScroll());

        m_pushConstants.invProj = m_camera.getInvProj();
        m_pushConstants.invView = m_camera.getInvView();
    }

    void onRender(const CommandBufferHandle& commandBuffer) override {
        ImGui::SliderInt("Test slider", &m_testInt, 0, 100);

        commandBuffer->bindDescriptorSet(m_pipeline, m_descSet);
        commandBuffer->bindPipeline(m_pipeline);
        commandBuffer->pushConstants(m_pipeline, &m_pushConstants);
        commandBuffer->traceRays(m_pipeline, Window::getWidth(), Window::getHeight(), 1);
        commandBuffer->copyImage(m_image, getCurrentColorImage(), vk::ImageLayout::eGeneral,
                                 vk::ImageLayout::ePresentSrcKHR);
    }

    std::vector<Vertex> m_vertices{{{-1, 0, 0}}, {{0, 1, 0}}, {{1, 0, 0}}};
    std::vector<uint32_t> m_indices{0, 1, 2};
    Mesh m_mesh;

    BottomAccelHandle m_bottomAccel;
    TopAccelHandle m_topAccel;
    ImageHandle m_image;

    DescriptorSetHandle m_descSet;
    RayTracingPipelineHandle m_pipeline;

    Camera m_camera;
    PushConstants m_pushConstants;
    int m_testInt = 0;
};

int main() {
    try {
        HelloApp app{};
        app.run();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}
