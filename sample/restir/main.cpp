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
    void Copy(vk::CommandBuffer commandBuffer, ResevImages& dst)
    {
        sample.setImageLayout(commandBuffer, vk::ImageLayout::eTransferSrcOptimal);
        weight.setImageLayout(commandBuffer, vk::ImageLayout::eTransferSrcOptimal);
        dst.sample.setImageLayout(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
        dst.weight.setImageLayout(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
        sample.copyToImage(commandBuffer, dst.sample);
        weight.copyToImage(commandBuffer, dst.weight);
        sample.setImageLayout(commandBuffer, vk::ImageLayout::eGeneral);
        weight.setImageLayout(commandBuffer, vk::ImageLayout::eGeneral);
        dst.sample.setImageLayout(commandBuffer, vk::ImageLayout::eGeneral);
        dst.weight.setImageLayout(commandBuffer, vk::ImageLayout::eGeneral);
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

    pipelines["GBuffer"].loadShaders(SHADER_DIR + "gbuffer/gbuffer.rgen",
                                     SHADER_DIR + "gbuffer/gbuffer.rmiss",
                                     SHADER_DIR + "gbuffer/gbuffer.rchit");

    pipelines["Uniform"].loadShaders(SHADER_DIR + "uniform_light/uniform_light.rgen",
                                     SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
                                     SHADER_DIR + "shadow_ray/shadow_ray.rchit");

    pipelines["WRS"].loadShaders(SHADER_DIR + "wrs/wrs.rgen",
                                 SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
                                 SHADER_DIR + "shadow_ray/shadow_ray.rchit");

    pipelines["InitResev"].loadShaders(SHADER_DIR + "restir/init_resev.rgen",
                                       SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
                                       SHADER_DIR + "shadow_ray/shadow_ray.rchit");

    pipelines["SpatialReuse"].loadShaders(SHADER_DIR + "restir/spatial_reuse.rgen",
                                          SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
                                          SHADER_DIR + "shadow_ray/shadow_ray.rchit");

    pipelines["TemporalReuse"].loadShaders(SHADER_DIR + "restir/temporal_reuse.rgen",
                                           SHADER_DIR + "shadow_ray/shadow_ray.rmiss",
                                           SHADER_DIR + "shadow_ray/shadow_ray.rchit");

    pipelines["Shading"].loadShaders(SHADER_DIR + "restir/shading.rgen",
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

    descSet->record("positionImage", gbuffers.position);
    descSet->record("normalImage", gbuffers.normal);
    descSet->record("diffuseImage", gbuffers.diffuse);
    descSet->record("emissionImage", gbuffers.emission);
    descSet->record("instanceIndexImage", gbuffers.instanceIndex);

    descSet->record("outputImage", outputImage.output);

    descSet->record("resevSampleImage", initedResevImages.sample);
    descSet->record("resevWeightImage", initedResevImages.weight);
    descSet->record("newResevSampleImage", reusedResevImages.sample);
    descSet->record("newResevWeightImage", reusedResevImages.weight);
    descSet->record("oldResevSampleImage", storedResevImages.sample);
    descSet->record("oldResevWeightImage", storedResevImages.weight);
    descSet->setup();

    for (auto& [name, pipeline] : pipelines) {
        pipeline.setup(sizeof(PushConstants));
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
        GUI::combo("Method", method, { "Uniform", "WRS", "ReSTIR" });
        GUI::sliderInt("Samples", pushConstants.samples, 1, 32);
        GUI::sliderInt("Iteration", iteration, 0, 4);
        GUI::checkbox("Temporal reuse", temporalReuse);

        scene.Update(0.1);
        pushConstants.invProj = scene.GetCamera().GetInvProj();
        pushConstants.invView = scene.GetCamera().GetInvView();
        pushConstants.frame++;

        Engine::Render(
            [&](auto commandBuffer) {
            auto width = Window::getWidth();
        auto height = Window::getHeight();
        pipelines["GBuffer"].run(commandBuffer, width, height, &pushConstants);

        if (method == Uniform) {
            pipelines["Uniform"].run(commandBuffer, width, height, &pushConstants);
        } else if (method == WRS) {
            pipelines["WRS"].run(commandBuffer, width, height, &pushConstants);
        } else if (method == ReSTIR) {
            pipelines["InitResev"].run(commandBuffer, width, height, &pushConstants);
            if (temporalReuse) {
                pipelines["TemporalReuse"].run(commandBuffer, width, height, &pushConstants);
                reusedResevImages.Copy(commandBuffer, initedResevImages);
            }
            initedResevImages.Copy(commandBuffer, storedResevImages);
            for (int i = 0; i < iteration; i++) {
                pipelines["SpatialReuse"].run(commandBuffer, width, height, &pushConstants);
                reusedResevImages.Copy(commandBuffer, initedResevImages);
            }
            pipelines["Shading"].run(commandBuffer, width, height, &pushConstants);
        }
        Context::copyToBackImage(commandBuffer, outputImage.output);
        });
    }
    Engine::Shutdown();
}
