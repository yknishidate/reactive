#include "Engine.hpp"

struct PushConstants
{
    glm::mat4 invView{ 1 };
    glm::mat4 invProj{ 1 };
};

int main()
{
    Engine::Init(750, 750);

    std::vector<Vertex> vertices{ {{-1, 0, 0}}, {{ 0, -1, 0}}, {{ 1, 0, 0}} };
    std::vector<Index> indices{ 0, 1, 2 };
    auto mesh = std::make_shared<Mesh>(vertices, indices);

    Scene scene{};
    scene.AddObject(mesh);
    scene.Setup();

    Image outputImage{ vk::Format::eB8G8R8A8Unorm };

    RayTracingPipeline pipeline{ };
    pipeline.LoadShaders(SHADER_DIR + "hello_raytracing.rgen",
                         SHADER_DIR + "hello_raytracing.rmiss",
                         SHADER_DIR + "hello_raytracing.rchit");
    pipeline.GetDescSet().Register("topLevelAS", scene.GetAccel());
    pipeline.GetDescSet().Register("outputImage", outputImage);
    pipeline.GetDescSet().Setup();
    pipeline.Setup(sizeof(PushConstants));

    int testInt = 0;
    while (Engine::Update()) {
        scene.Update(0.1f);
        GUI::SliderInt("Test slider", testInt, 0, 100);

        PushConstants pushConstants;
        pushConstants.invProj = scene.GetCamera().GetInvProj();
        pushConstants.invView = scene.GetCamera().GetInvView();

        Engine::Render([&](auto commandBuffer) {
            pipeline.Run(commandBuffer, Window::GetWidth(), Window::GetHeight(), &pushConstants);
        Context::CopyToBackImage(commandBuffer, outputImage); });
    }
    Engine::Shutdown();
}
