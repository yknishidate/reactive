#include "Engine.hpp"

struct PushConstants
{
    glm::mat4 invView{ 1 };
    glm::mat4 invProj{ 1 };
};

int main()
{
    try {
        Log::Init();
        Window::Init(750, 750);
        Context::Init();
        GUI::Init();

        std::vector<Vertex> vertices{ {{-1, 0, 0}}, {{ 0, -1, 0}}, {{ 1, 0, 0}} };
        std::vector<Index> indices{ 0, 1, 2 };
        Mesh mesh{ vertices, indices };
        Object object{ mesh };
        TopAccel topAccel{ object };
        Camera camera{ Window::GetWidth(), Window::GetHeight() };

        Image outputImage{ vk::Format::eB8G8R8A8Unorm };

        RayTracingPipeline pipeline{ };
        pipeline.LoadShaders(SHADER_DIR + "hello_raytracing.rgen",
                             SHADER_DIR + "hello_raytracing.rmiss",
                             SHADER_DIR + "hello_raytracing.rchit");
        pipeline.GetDescSet().Register("topLevelAS", topAccel);
        pipeline.GetDescSet().Register("outputImage", outputImage);
        pipeline.GetDescSet().Setup();
        pipeline.Setup(sizeof(PushConstants));

        int testInt = 0;
        while (!Window::ShouldClose()) {
            Window::PollEvents();
            GUI::StartFrame();
            camera.ProcessInput();
            GUI::SliderInt("Test slider", testInt, 0, 100);

            PushConstants pushConstants;
            pushConstants.invProj = camera.GetInvProj();
            pushConstants.invView = camera.GetInvView();

            Context::WaitNextFrame();
            vk::CommandBuffer commandBuffer = Context::BeginCommandBuffer();
            pipeline.Run(commandBuffer, Window::GetWidth(), Window::GetHeight(), &pushConstants);
            Context::CopyToBackImage(commandBuffer, outputImage);
            GUI::Render(commandBuffer);
            Context::Submit();
            Context::Present();
        }

        Context::GetDevice().waitIdle();
        GUI::Shutdown();
        Window::Shutdown();
    } catch (const std::exception& e) {
        Log::error(e.what());
    }
}
