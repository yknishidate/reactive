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

        //std::vector<Vertex> vertices{ {{-1, 0, 0}}, {{ 0, -1, 0}}, {{ 1, 0, 0}} };
        //std::vector<Index> indices{ 0, 1, 2 };
        //Mesh mesh{ vertices, indices };

        Swapchain swapchin{};
        GUI gui{ swapchin };
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
            gui.startFrame();
            gui.sliderInt("Test slider", testInt, 0, 100);
            camera.processInput();

            PushConstants pushConstants;
            pushConstants.invProj = camera.getInvProj();
            pushConstants.invView = camera.getInvView();

            swapchin.waitNextFrame();

            CommandBuffer commandBuffer = swapchin.beginCommandBuffer();
            commandBuffer.bindPipeline(pipeline);
            commandBuffer.pushConstants(&pushConstants);
            commandBuffer.traceRays(Window::getWidth(), Window::getHeight());
            commandBuffer.copyToBackImage(outputImage);
            commandBuffer.beginRenderPass();
            commandBuffer.drawGUI(gui);
            commandBuffer.endRenderPass();
            commandBuffer.submit();

            swapchin.present();
        }
        Context::waitIdle();
        Window::shutdown();
    } catch (const std::exception& e) {
        Log::error(e.what());
    }
}
