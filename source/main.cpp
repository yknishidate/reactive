#include "Engine.hpp"

int main()
{
    Engine::Init();

    Scene scene{ "../asset/crytek_sponza/sponza.obj",
                 glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec3{ 0.01f },
                 glm::vec3{ 0.0f, glm::radians(90.0f), 0.0f } };
    BoundingBox bbox = scene.GetBoundingBox();
    std::mt19937 mt{ std::random_device{}() };
    std::uniform_real_distribution<float> distX{ bbox.min.x, bbox.max.x };
    std::uniform_real_distribution<float> distY{ bbox.min.y, bbox.max.y };
    std::uniform_real_distribution<float> distZ{ bbox.min.z, bbox.max.z };

    PushConstants pushConstants;
    pushConstants.numLights = 100;
    for (int index = 0; index < pushConstants.numLights; index++) {
        const glm::vec3 position = glm::vec3{ distX(mt), distY(mt), distZ(mt) } / 2.5f;
        const glm::vec3 color{ 1.0f };
        scene.AddSphereLight(color, position, 0.1f);
    }
    scene.Setup();

    GBufferPipeline gbufferPipeline{ scene, sizeof(PushConstants) };

    UniformLightPipeline uniformLightPipeline;
    uniformLightPipeline.LoadShaders();
    uniformLightPipeline.RegisterScene(scene);
    uniformLightPipeline.RegisterGBuffers(gbufferPipeline.GetGBuffers());
    uniformLightPipeline.Setup(sizeof(PushConstants));

    WRSPipeline wrsPipeline;
    wrsPipeline.LoadShaders();
    wrsPipeline.RegisterScene(scene);
    wrsPipeline.RegisterGBuffers(gbufferPipeline.GetGBuffers());
    wrsPipeline.Setup(sizeof(PushConstants));

    pushConstants.invProj = scene.GetCamera().GetInvProj();
    pushConstants.invView = scene.GetCamera().GetInvView();
    pushConstants.frame = 0;

    int method = 0;
    constexpr int Uniform = 0;
    constexpr int WRS = 1;
    while (Engine::Update()) {
        UI::Combo("Method", method, { "Uniform", "WRS" });
        UI::SliderInt("Samples", pushConstants.samples, 1, 32);

        scene.Update(0.1);
        scene.ProcessInput();
        pushConstants.invProj = scene.GetCamera().GetInvProj();
        pushConstants.invView = scene.GetCamera().GetInvView();
        pushConstants.frame++;

        Engine::Render([&](vk::CommandBuffer commandBuffer) {
            uint32_t width = Window::GetWidth();
            uint32_t height = Window::GetHeight();
            gbufferPipeline.Run(commandBuffer, width, height, &pushConstants);

            if (method == Uniform) {
                uniformLightPipeline.Run(commandBuffer, width, height, &pushConstants);
                Context::CopyToBackImage(commandBuffer, uniformLightPipeline.GetOutputImage());
            } else if (method == WRS) {
                wrsPipeline.Run(commandBuffer, width, height, &pushConstants);
                Context::CopyToBackImage(commandBuffer, wrsPipeline.GetOutputImage());
            }
            UI::Render(commandBuffer); });
    }
    Engine::Shutdown();
}
