#include "Engine.hpp"

struct PushConstants
{
    glm::mat4 invView{ 1 };
    glm::mat4 invProj{ 1 };
};

int main()
{
    try {
        Log::init();
        Window::init(750, 750);
        Context::init();
        GUI::init();

        //std::vector<Vertex> vertices{ {{-1, 0, 0}}, {{ 0, -1, 0}}, {{ 1, 0, 0}} };
        //std::vector<Index> indices{ 0, 1, 2 };
        //Mesh mesh{ vertices, indices };

        Mesh mesh{ "bunny.obj" };
        Object object{ mesh };
        TopAccel topAccel{ object };
        Camera camera{ Window::getWidth(), Window::getHeight() };

        Image outputImage{ vk::Format::eB8G8R8A8Unorm };

        RayTracingPipeline pipeline{ };
        pipeline.loadShaders(SHADER_DIR + "hello_raytracing.rgen",
                             SHADER_DIR + "hello_raytracing.rmiss",
                             SHADER_DIR + "hello_raytracing.rchit");
        pipeline.getDescSet().record("topLevelAS", topAccel);
        pipeline.getDescSet().record("outputImage", outputImage);
        pipeline.getDescSet().setup();
        pipeline.setup(sizeof(PushConstants));

        int testInt = 0;
        while (!Window::shouldClose()) {
            Window::pollEvents();
            GUI::startFrame();
            GUI::sliderInt("Test slider", testInt, 0, 100);
            camera.processInput();

            PushConstants pushConstants;
            pushConstants.invProj = camera.getInvProj();
            pushConstants.invView = camera.getInvView();

            Context::waitNextFrame();
            vk::CommandBuffer commandBuffer = Context::beginCommandBuffer();
            pipeline.bind(commandBuffer);
            pipeline.pushConstants(commandBuffer, &pushConstants);
            pipeline.traceRays(commandBuffer, Window::getWidth(), Window::getHeight());
            Context::copyToBackImage(commandBuffer, outputImage);

            Context::beginRenderPass();
            GUI::render(commandBuffer);
            Context::endRenderPass();

            Context::submit();
            Context::present();
        }

        Context::waitIdle();
        GUI::shutdown();
        Window::shutdown();
    } catch (const std::exception& e) {
        Log::error(e.what());
    }
}
