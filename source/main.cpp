#include "Engine.hpp"

int main()
{
    Engine::Init();

    Scene scene{ "../asset/crytek_sponza/sponza.obj",
                 glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec3{ 0.01f },
                 glm::vec3{ 0.0f, glm::radians(90.0f), 0.0f } };

    // Add random lights
    BoundingBox bbox = scene.GetBoundingBox();
    std::mt19937 mt{};
    std::uniform_real_distribution distX{ bbox.min.x, bbox.max.x };
    std::uniform_real_distribution distY{ bbox.min.y, bbox.max.y };
    std::uniform_real_distribution distZ{ bbox.min.z, bbox.max.z };

    for (int index = 0; index < 100; index++) {
        const glm::vec3 position = glm::vec3{ distX(mt), distY(mt), distZ(mt) } / 2.5f;
        const glm::vec3 color{ 1.0f };
        scene.AddSphereLight(color, position, 0.1f);
    }
    scene.Setup();

    GBufferPipeline gbufferPipeline{ scene, sizeof(PushConstants) };
    UniformLightPipeline uniformLightPipeline{ scene, gbufferPipeline.GetGBuffers(), sizeof(PushConstants) };
    WRSPipeline wrsPipeline{ scene, gbufferPipeline.GetGBuffers(), sizeof(PushConstants) };
    InitResevPipeline initResevPipeline{ scene, gbufferPipeline.GetGBuffers(), sizeof(PushConstants) };
    ReuseResevPipeline reuseResevPipeline{ scene, gbufferPipeline.GetGBuffers(), initResevPipeline.GetResevImages(), sizeof(PushConstants) };
    ShadingPipeline shadingPipeline{ scene, gbufferPipeline.GetGBuffers(), initResevPipeline.GetResevImages(), sizeof(PushConstants) };

    PushConstants pushConstants;
    pushConstants.numLights = scene.GetNumSphereLights();
    pushConstants.invProj = scene.GetCamera().GetInvProj();
    pushConstants.invView = scene.GetCamera().GetInvView();
    pushConstants.frame = 0;

    int method = 2;
    int iteration = 0;
    constexpr int Uniform = 0;
    constexpr int WRS = 1;
    constexpr int ReSTIR = 2;
    while (Engine::Update()) {
        GUI::Combo("Method", method, { "Uniform", "WRS", "ReSTIR" });
        GUI::SliderInt("Samples", pushConstants.samples, 1, 32);
        GUI::SliderInt("Iteration", iteration, 0, 4);

        scene.Update(0.1);
        pushConstants.invProj = scene.GetCamera().GetInvProj();
        pushConstants.invView = scene.GetCamera().GetInvView();
        pushConstants.frame++;

        Engine::Render(
            [&]() {
                gbufferPipeline.Run(&pushConstants);

                if (method == Uniform) {
                    uniformLightPipeline.Run(&pushConstants);
                    Context::CopyToBackImage(uniformLightPipeline.GetOutputImage());
                } else if (method == WRS) {
                    wrsPipeline.Run(&pushConstants);
                    Context::CopyToBackImage(wrsPipeline.GetOutputImage());
                } else if (method == ReSTIR) {
                    initResevPipeline.Run(&pushConstants);
                    for (int i = 0; i < iteration; i++) {
                        reuseResevPipeline.Run(&pushConstants);
                        reuseResevPipeline.CopyToResevImages(initResevPipeline.GetResevImages());
                    }
                    shadingPipeline.Run(&pushConstants);
                    Context::CopyToBackImage(shadingPipeline.GetOutputImage());
                }
            });
    }
    Engine::Shutdown();
}
