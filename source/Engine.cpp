#include <random>
#include <functional>
#include "Engine.hpp"
#include "Input.hpp"
#include "Loader.hpp"
#include "Scene.hpp"
#include "Object.hpp"
#include "UI.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace
{
    glm::vec3 ToRGB(glm::vec3 c)
    {
        glm::vec4 K = glm::vec4{ 1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0 };
        glm::vec3 p = glm::abs(fract(c.xxx + K.xyz) * 6.0f - K.www);
        return c.z * glm::mix(glm::vec3{ K.xxx }, glm::clamp(p - K.xxx, 0.0f, 1.0f), c.y);
    }

    glm::vec3 GetColorFromHue(float hue)
    {
        return ToRGB(glm::vec3{ hue, 0.7, 1.0 });
    }
}

void Engine::Init()
{
    spdlog::set_pattern("[%^%l%$] %v");
    spdlog::info("Engine::Init()");

    Window::Init(1920, 1080, "../asset/Vulkan.png");
    Context::Init();
    UI::Init();

    //// Create resources
    //inputImage = Image{ Window::GetWidth(), Window::GetHeight(), vk::Format::eB8G8R8A8Unorm };
    //outputImage = Image{ Window::GetWidth(), Window::GetHeight(), vk::Format::eB8G8R8A8Unorm };
    //reservoirSampleImage = Image{ Window::GetWidth(), Window::GetHeight(), vk::Format::eR16Uint };
    //reservoirWeightImage = Image{ Window::GetWidth(), Window::GetHeight(), vk::Format::eR32Sfloat };
    //denoisedImage = Image{ Window::GetWidth(), Window::GetHeight(), vk::Format::eB8G8R8A8Unorm };

    // Load scene
    {
        scene = std::make_unique<Scene>("../asset/crytek_sponza/sponza.obj",
                                        glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec3{ 0.01f },
                                        glm::vec3{ 0.0f, glm::radians(90.0f), 0.0f });
        BoundingBox bbox = scene->GetBoundingBox();
        std::mt19937 mt{ std::random_device{}() };
        std::uniform_real_distribution<float> distX{ bbox.min.x, bbox.max.x };
        std::uniform_real_distribution<float> distY{ bbox.min.y, bbox.max.y };
        std::uniform_real_distribution<float> distZ{ bbox.min.z, bbox.max.z };

        //std::uniform_real_distribution<float> distHue{ 0.0f, 1.0f };
        //std::uniform_real_distribution<float> distStrength{ 80.0f, 160.0f };
        //pushConstants.numLights = 10;
        //for (int index = 0; index < pushConstants.numLights; index++) {
        //    const glm::vec3 position = glm::vec3{ distX(mt), distY(mt), distZ(mt) } / 2.5f;
        //    const glm::vec3 color = GetColorFromHue(distHue(mt)) * distStrength(mt);
        //    scene->AddSphereLight(color, position, 0.1f);
        //}

        pushConstants.numLights = 5;
        for (int index = 0; index < pushConstants.numLights; index++) {
            const glm::vec3 position = glm::vec3{ distX(mt), distY(mt), distZ(mt) } / 2.5f;
            const glm::vec3 color{ 5.0f };
            scene->AddSphereLight(color, position, 0.1f);
        }

        //pushConstants.numLights = 1;
        //scene->AddSphereLight(glm::vec3{ 10.0f }, glm::vec3{ 0.0f }, 0.5f);
    }
    {
        //scene = std::make_unique<Scene>("../asset/CornellBox/CornellBox-Glossy.obj", glm::vec3{ 0.0f, 0.75f, 0.0f });
        //scene->AddSphereLight(glm::vec3{ 10.0f }, glm::vec3{ 0.0f, -0.5f, 0.0f }, 0.2f);
        //pushConstants.numLights = 1;
    }

    scene->Setup();

    // Create pipelines
    //ptPipeline.LoadShaders("../shader/pathtracing/pathtracing.rgen",
    //                       "../shader/pathtracing/pathtracing.rmiss",
    //                       "../shader/pathtracing/pathtracing.rchit");
    //ptPipeline.Register("inputImage", inputImage);
    //ptPipeline.Register("outputImage", outputImage);
    //ptPipeline.Register("samplers", outputImage); // Dummy
    //ptPipeline.Register("topLevelAS", scene->GetAccel());
    //ptPipeline.Register("samplers", scene->GetTextures());
    //ptPipeline.Register("Addresses", scene->GetAddressBuffer());
    //ptPipeline.Setup(sizeof(PushConstants));

    //neePipeline.LoadShaders("../shader/pathtracing/nee.rgen",
    //                        "../shader/pathtracing/pathtracing.rmiss",
    //                        "../shader/pathtracing/nee.rchit");
    //neePipeline.Register("inputImage", inputImage);
    //neePipeline.Register("outputImage", outputImage);
    //neePipeline.Register("samplers", outputImage); // Dummy
    //neePipeline.Register("topLevelAS", scene->GetAccel());
    //neePipeline.Register("samplers", scene->GetTextures());
    //neePipeline.Register("Addresses", scene->GetAddressBuffer());
    //neePipeline.Setup(sizeof(PushConstants));

    //srisPipeline.LoadShaders("../shader/pathtracing/nee.rgen",
    //                         "../shader/pathtracing/pathtracing.rmiss",
    //                         "../shader/pathtracing/streaming_ris.rchit");
    //srisPipeline.Register("inputImage", inputImage);
    //srisPipeline.Register("outputImage", outputImage);
    //srisPipeline.Register("samplers", outputImage); // Dummy
    //srisPipeline.Register("topLevelAS", scene->GetAccel());
    //srisPipeline.Register("samplers", scene->GetTextures());
    //srisPipeline.Register("Addresses", scene->GetAddressBuffer());
    //srisPipeline.Setup(sizeof(PushConstants));

    //descSet.Allocate("../shader/restir/init_reservoir.rgen");

    gbufferPipeline.LoadShaders();
    gbufferPipeline.RegisterAccel(scene->GetAccel());
    gbufferPipeline.RegisterTextures(scene->GetTextures());
    gbufferPipeline.RegisterBufferAddresses(scene->GetAddressBuffer());
    gbufferPipeline.Setup(sizeof(PushConstants));

    uniformLightPipeline.LoadShaders();
    uniformLightPipeline.RegisterAccel(scene->GetAccel());
    uniformLightPipeline.RegisterTextures(scene->GetTextures());
    uniformLightPipeline.RegisterBufferAddresses(scene->GetAddressBuffer());
    uniformLightPipeline.RegisterGBuffers(gbufferPipeline.GetGBuffers());
    uniformLightPipeline.Setup(sizeof(PushConstants));

    //initReservoirPipeline.LoadShaders("../shader/restir/init_reservoir.rgen",
    //                                  "../shader/restir/init_reservoir.rmiss",
    //                                  "../shader/restir/init_reservoir.rchit");
    //initReservoirPipeline.Register("inputImage", inputImage);
    //initReservoirPipeline.Register("outputImage", outputImage);
    //initReservoirPipeline.Register("reservoirSampleImage", reservoirSampleImage);
    //initReservoirPipeline.Register("reservoirWeightImage", reservoirWeightImage);
    //initReservoirPipeline.Register("indexImage", indexImage);
    //initReservoirPipeline.Register("diffuseImage", diffuseImage);
    //initReservoirPipeline.Register("emissionImage", emissionImage);
    //initReservoirPipeline.Register("posImage", posImage);
    //initReservoirPipeline.Register("normalImage", normalImage);
    //initReservoirPipeline.Register("samplers", outputImage); // Dummy
    //initReservoirPipeline.Register("topLevelAS", scene->GetAccel());
    //initReservoirPipeline.Register("samplers", scene->GetTextures());
    //initReservoirPipeline.Register("Addresses", scene->GetAddressBuffer());
    //initReservoirPipeline.Setup(sizeof(PushConstants));

    //shadingPipeline.LoadShaders("../shader/restir/shading.rgen",
    //                            "../shader/restir/shading.rmiss",
    //                            "../shader/restir/shading.rchit");
    //shadingPipeline.Register("inputImage", inputImage);
    //shadingPipeline.Register("outputImage", outputImage);
    //shadingPipeline.Register("reservoirSampleImage", reservoirSampleImage);
    //shadingPipeline.Register("reservoirWeightImage", reservoirWeightImage);
    //shadingPipeline.Register("indexImage", indexImage);
    //shadingPipeline.Register("diffuseImage", diffuseImage);
    //shadingPipeline.Register("emissionImage", emissionImage);
    //shadingPipeline.Register("posImage", posImage);
    //shadingPipeline.Register("normalImage", normalImage);
    //shadingPipeline.Register("samplers", outputImage); // Dummy
    //shadingPipeline.Register("topLevelAS", scene->GetAccel());
    //shadingPipeline.Register("samplers", scene->GetTextures());
    //shadingPipeline.Register("Addresses", scene->GetAddressBuffer());
    //shadingPipeline.Setup(sizeof(PushConstants));

    //medianPipeline.LoadShaders("../shader/denoise/median.comp");
    //medianPipeline.Register("inputImage", outputImage);
    //medianPipeline.Register("outputImage", denoisedImage);
    //medianPipeline.Setup(sizeof(PushConstants));

    // Create push constants
    pushConstants.invProj = glm::inverse(scene->GetCamera().GetProj());
    pushConstants.invView = glm::inverse(scene->GetCamera().GetView());
    pushConstants.frame = 0;
}

void Engine::Run()
{
    bool accumulation = false;
    bool importance = false;
    int denoise = 0;

    spdlog::info("Engine::Run()");
    while (!Window::ShouldClose()) {
        Window::PollEvents();
        Input::Update();

        // Setup UI
        UI::StartFrame();
        UI::Checkbox("Accumulation", accumulation);
        bool refresh = false;
        refresh |= UI::Checkbox("Importance sampling", importance);
        refresh |= UI::Combo("Method", pushConstants.nee, { "PT", "Uniform", "RIS", "WRS", "SRIS", "ReSTIR" });
        UI::Combo("Denoise", denoise, { "Off", "Median" });
        UI::Checkbox("Reuse", pushConstants.reuse);
        refresh |= UI::SliderInt("Depth", pushConstants.depth, 1, 8);
        refresh |= UI::SliderInt("Samples", pushConstants.samples, 1, 32);
        refresh |= UI::ColorPicker4("Sky color", pushConstants.skyColor);
        UI::Prepare();

        // Scene update
        //scene->Update(0.1);

        // Update push constants
        scene->ProcessInput();
        pushConstants.invProj = glm::inverse(scene->GetCamera().GetProj());
        pushConstants.invView = glm::inverse(scene->GetCamera().GetView());
        pushConstants.importance = static_cast<int>(importance);
        if (refresh || !accumulation || scene->GetCamera().CheckDirtyAndClean()) {
            pushConstants.frame = 0;
        } else {
            pushConstants.frame++;
        }

        // Render
        if (!Window::IsMinimized()) {
            Context::WaitNextFrame();
            vk::CommandBuffer commandBuffer = Context::BeginCommandBuffer();

            uint32_t width = Window::GetWidth();
            uint32_t height = Window::GetHeight();
            //if (pushConstants.nee == 0) {
            //    ptPipeline.Run(commandBuffer, width, height, &pushConstants);
            //} else if (pushConstants.nee == 1 || pushConstants.nee == 2 || pushConstants.nee == 3) {
            //    neePipeline.Run(commandBuffer, width, height, &pushConstants);
            //} else if (pushConstants.nee == 4) {
            //    srisPipeline.Run(commandBuffer, width, height, &pushConstants);
            //}
            //initReservoirPipeline.Run(commandBuffer, width, height, &pushConstants);
            //shadingPipeline.Run(commandBuffer, width, height, &pushConstants);
            gbufferPipeline.Run(commandBuffer, width, height, &pushConstants);
            uniformLightPipeline.Run(commandBuffer, width, height, &pushConstants);

            //const GBuffers& gbuffers = gbufferPipeline.GetGBuffers();
            //Context::CopyToBackImage(commandBuffer, gbuffers.diffuse);

            const Image& outputImage = uniformLightPipeline.GetOutputImage();
            Context::CopyToBackImage(commandBuffer, outputImage);

            UI::Render(commandBuffer);
            Context::Submit();
            Context::Present();
        }
    }
    Context::GetDevice().waitIdle();
    UI::Shutdown();
    Window::Shutdown();
}
