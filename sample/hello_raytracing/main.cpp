#include "Engine.hpp"

struct PushConstants
{
    glm::mat4 invView{ 1 };
    glm::mat4 invProj{ 1 };
};

int main()
{
    Engine::Init();

    std::vector<Vertex> vertices = {
        {glm::vec3{-1,  0, 0}},
        {glm::vec3{ 0, -1, 0}},
        {glm::vec3{ 1,  0, 0}},
    };
    std::vector<Index> indices = { 0, 1, 2 };
    auto mesh = std::make_shared<Mesh>(vertices, indices);

    Scene scene{};
    scene.AddObject(mesh);
    scene.Setup();

    auto descSet = std::make_shared<DescriptorSet>();

    Image outputImage{ vk::Format::eB8G8R8A8Unorm };

    RayTracingPipeline pipeline{ descSet };
    pipeline.LoadShaders(SHADER_DIR + "hello_raytracing.rgen",
                         SHADER_DIR + "hello_raytracing.rmiss",
                         SHADER_DIR + "hello_raytracing.rchit");

    descSet->Register("topLevelAS", scene.GetAccel());
    descSet->Register("outputImage", outputImage);
    pipeline.Setup(sizeof(PushConstants));

    while (Engine::Update()) {
        scene.Update(0.1);

        PushConstants pushConstants;
        pushConstants.invProj = scene.GetCamera().GetInvProj();
        pushConstants.invView = scene.GetCamera().GetInvView();

        Engine::Render(
            [&]() {
                pipeline.Run(&pushConstants);
                Context::CopyToBackImage(outputImage);
            });
    }
    Engine::Shutdown();
}
