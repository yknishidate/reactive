#include "Engine.hpp"
#include "Pipeline.hpp"

struct PushConstants
{
    glm::mat4 invView{ 1 };
    glm::mat4 invProj{ 1 };
    int frame = 0;
    int depth = 1;
    int samples = 1;
    int numLights = 0;
    glm::vec4 skyColor{ 0, 0, 0, 0 };
};

int main()
{
    Engine::Init();

    Scene scene{ "crytek_sponza/sponza.obj",
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

    auto descSet = std::make_shared<DescriptorSet>();

    std::unordered_map<std::string, RayTracingPipeline> pipelines;
    pipelines.insert({ "GBuffer", RayTracingPipeline{ descSet } });
    pipelines.insert({ "Uniform", RayTracingPipeline{ descSet } });
    //pipelines.insert({ "WRS", RayTracingPipeline{ descSet } });
    //pipelines.insert({ "InitResev", RayTracingPipeline{ descSet } });
    //pipelines.insert({ "SpatialReuse", RayTracingPipeline{ descSet } });
    //pipelines.insert({ "TemporalReuse", RayTracingPipeline{ descSet } });
    //pipelines.insert({ "Shading", RayTracingPipeline{ descSet } });

    pipelines["GBuffer"].LoadShaders(SHADER_DIR + "gbuffer/gbuffer.rgen",
                                     SHADER_DIR + "gbuffer/gbuffer.rmiss",
                                     SHADER_DIR + "gbuffer/gbuffer.rchit");

    pipelines["Uniform"].LoadShaders(SHADER_DIR + "uniform_light/uniform_light.rgen",
                                     SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
                                     SHADER_DIR + "shadow_ray/shadow_ray.rchit");

    //pipelines["WRS"].LoadShaders(SHADER_DIR + "wrs/wrs.rgen",
    //                             SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
    //                             SHADER_DIR + "shadow_ray/shadow_ray.rchit");

    //pipelines["InitResev"].LoadShaders(SHADER_DIR + "restir/init_resev.rgen",
    //                                   SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
    //                                   SHADER_DIR + "shadow_ray/shadow_ray.rchit");

    //pipelines["SpatialReuse"].LoadShaders(SHADER_DIR + "restir/spatial_reuse.rgen",
    //                                      SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
    //                                      SHADER_DIR + "shadow_ray/shadow_ray.rchit");

    //pipelines["TemporalReuse"].LoadShaders(SHADER_DIR + "restir/temporal_reuse.rgen",
    //                                       SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
    //                                       SHADER_DIR + "shadow_ray/shadow_ray.rchit");

    //pipelines["Shading"].LoadShaders(SHADER_DIR + "restir/shading.rgen",
    //                                 SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
    //                                 SHADER_DIR + "shadow_ray/shadow_ray.rchit");

    GBuffers gbuffers;
    OutputImage outputImage;
    //ResevImages initedResevImages;
    //ResevImages spatialReusedResevImages;
    //ResevImages temporalReusedResevImages;
    //ResevImages storedResevImages;

    descSet->Register("topLevelAS", scene.GetAccel());
    descSet->Register("samplers", scene.GetTextures());
    descSet->Register("Addresses", scene.GetAddressBuffer());

    descSet->Register("positionImage", gbuffers.position);
    descSet->Register("normalImage", gbuffers.normal);
    descSet->Register("diffuseImage", gbuffers.diffuse);
    descSet->Register("emissionImage", gbuffers.emission);
    descSet->Register("instanceIndexImage", gbuffers.instanceIndex);

    descSet->Register("outputImage", outputImage.outputImage);

    for (auto& [name, pipeline] : pipelines) {
        pipeline.Setup(sizeof(PushConstants));
    }

    //GBufferPipeline gbufferPipeline{ scene, sizeof(PushConstants) };
    //UniformLightPipeline uniformLightPipeline{ scene, gbufferPipeline.GetGBuffers(), sizeof(PushConstants) };
    //WRSPipeline wrsPipeline{ scene, gbufferPipeline.GetGBuffers(), sizeof(PushConstants) };
    //InitResevPipeline initResevPipeline{ scene, gbufferPipeline.GetGBuffers(), sizeof(PushConstants) };
    //SpatialReusePipeline spatialReusePipeline{ scene, gbufferPipeline.GetGBuffers(), initResevPipeline.GetResevImages(), sizeof(PushConstants) };
    //TemporalReusePipeline temporalReusePipeline{ scene, gbufferPipeline.GetGBuffers(), initResevPipeline.GetResevImages(), sizeof(PushConstants) };
    //ShadingPipeline shadingPipeline{ scene, gbufferPipeline.GetGBuffers(), initResevPipeline.GetResevImages(), sizeof(PushConstants) };

    PushConstants pushConstants;
    pushConstants.numLights = scene.GetNumSphereLights();
    pushConstants.invProj = scene.GetCamera().GetInvProj();
    pushConstants.invView = scene.GetCamera().GetInvView();
    pushConstants.frame = 0;

    int method = 2;
    int iteration = 0;
    bool temporalReuse = false;
    constexpr int Uniform = 0;
    constexpr int WRS = 1;
    constexpr int ReSTIR = 2;
    while (Engine::Update()) {
        GUI::Combo("Method", method, { "Uniform", "WRS", "ReSTIR" });
        GUI::SliderInt("Samples", pushConstants.samples, 1, 32);
        GUI::SliderInt("Iteration", iteration, 0, 4);
        GUI::Checkbox("Temporal reuse", temporalReuse);

        scene.Update(0.1);
        pushConstants.invProj = scene.GetCamera().GetInvProj();
        pushConstants.invView = scene.GetCamera().GetInvView();
        pushConstants.frame++;

        Engine::Render(
            [&]() {
                pipelines["GBuffer"].Run(&pushConstants);
                pipelines["Uniform"].Run(&pushConstants);
                Context::CopyToBackImage(outputImage.outputImage);

                //if (method == Uniform) {
                //    uniformLightPipeline.Run(&pushConstants);
                //    Context::CopyToBackImage(uniformLightPipeline.GetOutputImage());
                //} else if (method == WRS) {
                //    wrsPipeline.Run(&pushConstants);
                //    Context::CopyToBackImage(wrsPipeline.GetOutputImage());
                //} else if (method == ReSTIR) {
                //    initResevPipeline.Run(&pushConstants);
                //    if (temporalReuse) {
                //        temporalReusePipeline.Run(&pushConstants);
                //        temporalReusePipeline.CopyToResevImages(initResevPipeline.GetResevImages());
                //    }
                //    temporalReusePipeline.CopyFromResevImages(initResevPipeline.GetResevImages());
                //    for (int i = 0; i < iteration; i++) {
                //        spatialReusePipeline.Run(&pushConstants);
                //        spatialReusePipeline.CopyToResevImages(initResevPipeline.GetResevImages());
                //    }
                //    shadingPipeline.Run(&pushConstants);
                //    Context::CopyToBackImage(shadingPipeline.GetOutputImage());
                //}
            });
    }
    Engine::Shutdown();
}
