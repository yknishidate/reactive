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

void Engine::Init()
{
    spdlog::set_pattern("[%^%l%$] %v");
    spdlog::info("Engine::Init()");

    Window::Init(1920, 1080);
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

        pushConstants.numLights = 50;
        for (int index = 0; index < pushConstants.numLights; index++) {
            const glm::vec3 position = glm::vec3{ distX(mt), distY(mt), distZ(mt) } / 2.5f;
            const glm::vec3 color{ 1.0f };
            scene->AddSphereLight(color, position, 0.1f);
        }
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

    wrsPipeline.LoadShaders();
    wrsPipeline.RegisterAccel(scene->GetAccel());
    wrsPipeline.RegisterTextures(scene->GetTextures());
    wrsPipeline.RegisterBufferAddresses(scene->GetAddressBuffer());
    wrsPipeline.RegisterGBuffers(gbufferPipeline.GetGBuffers());
    wrsPipeline.Setup(sizeof(PushConstants));

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
    int method = 0;
    constexpr int Uniform = 0;
    constexpr int WRS = 1;

    spdlog::info("Engine::Run()");
    while (!Window::ShouldClose()) {
        Window::PollEvents();
        Input::Update();

        // Setup UI
        UI::StartFrame();
        UI::Combo("Method", method, { "Uniform", "WRS" });
        UI::Prepare();

        // Scene update
        //scene->Update(0.1);

        // Update push constants
        scene->ProcessInput();
        pushConstants.invProj = glm::inverse(scene->GetCamera().GetProj());
        pushConstants.invView = glm::inverse(scene->GetCamera().GetView());
        pushConstants.frame++;

        // Render
        if (!Window::IsMinimized()) {
            Context::WaitNextFrame();
            vk::CommandBuffer commandBuffer = Context::BeginCommandBuffer();

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

            UI::Render(commandBuffer);
            Context::Submit();
            Context::Present();
        }
    }
    Context::GetDevice().waitIdle();
    UI::Shutdown();
    Window::Shutdown();
}
