#include "Engine.hpp"

struct PushConstants
{
    glm::mat4 invView{ 1 };
    glm::mat4 invProj{ 1 };
};

int main()
{
    Engine::Init();

    Scene scene{ "crytek_sponza/sponza.obj",
                 glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec3{ 0.01f },
                 glm::vec3{ 0.0f, glm::radians(90.0f), 0.0f } };
    scene.Setup();

    auto descSet = std::make_shared<DescriptorSet>();

    Image outputImage{ vk::Format::eB8G8R8A8Unorm };
    RayTracingPipeline pipeline{ descSet };

    pipeline.LoadShaders(SHADER_DIR + "hello_raytracing.rgen",
                         SHADER_DIR + "hello_raytracing.rmiss",
                         SHADER_DIR + "hello_raytracing.rchit");

    descSet->Register("topLevelAS", scene.GetAccel());
    descSet->Register("samplers", scene.GetTextures());
    descSet->Register("Addresses", scene.GetAddressBuffer());
    descSet->Register("outputImage", outputImage);
    pipeline.Setup(sizeof(PushConstants));

    PushConstants pushConstants;
    pushConstants.invProj = scene.GetCamera().GetInvProj();
    pushConstants.invView = scene.GetCamera().GetInvView();

    while (Engine::Update()) {
        scene.Update(0.1);
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
