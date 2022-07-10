#include "Engine.hpp"

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

struct OutputImage
{
    Image output{ vk::Format::eB8G8R8A8Unorm };
};

struct GBuffers
{
    Image position{ vk::Format::eR16G16B16A16Sfloat };
    Image normal{ vk::Format::eR16G16B16A16Sfloat };
    Image diffuse{ vk::Format::eB8G8R8A8Unorm };
    Image emission{ vk::Format::eR16G16B16A16Sfloat };
    Image instanceIndex{ vk::Format::eR8G8B8A8Uint };
};

struct ResevImages
{
    void Copy(ResevImages& dst)
    {
        sample.CopyToImage(dst.sample);
        weight.CopyToImage(dst.weight);
    }

    Image sample{ vk::Format::eR32G32Uint };
    Image weight{ vk::Format::eR16G16Sfloat };
};

int main()
{
    Engine::Init();

    Scene scene{ "crytek_sponza/sponza.obj",
                 glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec3{ 0.01f },
                 glm::vec3{ 0.0f, glm::radians(90.0f), 0.0f } };

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
    pipelines.insert({ "WRS", RayTracingPipeline{ descSet } });
    pipelines.insert({ "InitResev", RayTracingPipeline{ descSet } });
    pipelines.insert({ "SpatialReuse", RayTracingPipeline{ descSet } });
    pipelines.insert({ "TemporalReuse", RayTracingPipeline{ descSet } });
    pipelines.insert({ "Shading", RayTracingPipeline{ descSet } });

    pipelines["GBuffer"].LoadShaders(SHADER_DIR + "gbuffer/gbuffer.rgen",
                                     SHADER_DIR + "gbuffer/gbuffer.rmiss",
                                     SHADER_DIR + "gbuffer/gbuffer.rchit");

    pipelines["Uniform"].LoadShaders(SHADER_DIR + "uniform_light/uniform_light.rgen",
                                     SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
                                     SHADER_DIR + "shadow_ray/shadow_ray.rchit");

    pipelines["WRS"].LoadShaders(SHADER_DIR + "wrs/wrs.rgen",
                                 SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
                                 SHADER_DIR + "shadow_ray/shadow_ray.rchit");

    pipelines["InitResev"].LoadShaders(SHADER_DIR + "restir/init_resev.rgen",
                                       SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
                                       SHADER_DIR + "shadow_ray/shadow_ray.rchit");

    pipelines["SpatialReuse"].LoadShaders(SHADER_DIR + "restir/spatial_reuse.rgen",
                                          SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
                                          SHADER_DIR + "shadow_ray/shadow_ray.rchit");

    pipelines["TemporalReuse"].LoadShaders(SHADER_DIR + "restir/temporal_reuse.rgen",
                                           SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
                                           SHADER_DIR + "shadow_ray/shadow_ray.rchit");

    pipelines["Shading"].LoadShaders(SHADER_DIR + "restir/shading.rgen",
                                     SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
                                     SHADER_DIR + "shadow_ray/shadow_ray.rchit");

    GBuffers gbuffers;
    OutputImage outputImage;
    ResevImages initedResevImages;
    ResevImages reusedResevImages;
    ResevImages storedResevImages;

    descSet->Register("topLevelAS", scene.GetAccel());
    descSet->Register("samplers", scene.GetTextures());
    descSet->Register("Addresses", scene.GetAddressBuffer());

    descSet->Register("positionImage", gbuffers.position);
    descSet->Register("normalImage", gbuffers.normal);
    descSet->Register("diffuseImage", gbuffers.diffuse);
    descSet->Register("emissionImage", gbuffers.emission);
    descSet->Register("instanceIndexImage", gbuffers.instanceIndex);

    descSet->Register("outputImage", outputImage.output);

    descSet->Register("resevSampleImage", initedResevImages.sample);
    descSet->Register("resevWeightImage", initedResevImages.weight);
    descSet->Register("newResevSampleImage", reusedResevImages.sample);
    descSet->Register("newResevWeightImage", reusedResevImages.weight);
    descSet->Register("oldResevSampleImage", storedResevImages.sample);
    descSet->Register("oldResevWeightImage", storedResevImages.weight);
    descSet->Setup();

    for (auto& [name, pipeline] : pipelines) {
        pipeline.Setup(sizeof(PushConstants));
    }

    PushConstants pushConstants;
    pushConstants.numLights = scene.GetNumSphereLights();
    pushConstants.invProj = scene.GetCamera().GetInvProj();
    pushConstants.invView = scene.GetCamera().GetInvView();
    pushConstants.frame = 0;

    int method = 0;
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

                if (method == Uniform) {
                    pipelines["Uniform"].Run(&pushConstants);
                } else if (method == WRS) {
                    pipelines["WRS"].Run(&pushConstants);
                } else if (method == ReSTIR) {
                    pipelines["InitResev"].Run(&pushConstants);
                    if (temporalReuse) {
                        pipelines["TemporalReuse"].Run(&pushConstants);
                        reusedResevImages.Copy(initedResevImages);
                    }
                    initedResevImages.Copy(storedResevImages);
                    for (int i = 0; i < iteration; i++) {
                        pipelines["SpatialReuse"].Run(&pushConstants);
                        reusedResevImages.Copy(initedResevImages);
                    }
                    pipelines["Shading"].Run(&pushConstants);
                }
                outputImage.output.CopyToBackImage();
            });
    }
    Engine::Shutdown();
}
