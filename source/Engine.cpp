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

    {
        scene = Scene{ "../asset/crytek_sponza/sponza.obj",
                      glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec3{ 0.01f },
                      glm::vec3{ 0.0f, glm::radians(90.0f), 0.0f } };
        BoundingBox bbox = scene.GetBoundingBox();
        std::mt19937 mt{ std::random_device{}() };
        std::uniform_real_distribution<float> distX{ bbox.min.x, bbox.max.x };
        std::uniform_real_distribution<float> distY{ bbox.min.y, bbox.max.y };
        std::uniform_real_distribution<float> distZ{ bbox.min.z, bbox.max.z };

        pushConstants.numLights = 20;
        for (int index = 0; index < pushConstants.numLights; index++) {
            const glm::vec3 position = glm::vec3{ distX(mt), distY(mt), distZ(mt) } / 2.5f;
            const glm::vec3 color{ 1.0f };
            scene.AddSphereLight(color, position, 0.1f);
        }
    }
    {
        //scene = std::make_unique<Scene>("../asset/CornellBox/CornellBox-Glossy.obj", glm::vec3{ 0.0f, 0.75f, 0.0f });
        //scene->AddSphereLight(glm::vec3{ 1.0f }, glm::vec3{ 0.0f, -0.5f, 0.0f }, 0.1f);
        //pushConstants.numLights = 1;
    }

    scene.Setup();

    gbufferPipeline.LoadShaders();
    gbufferPipeline.RegisterScene(scene);
    gbufferPipeline.Setup(sizeof(PushConstants));

    uniformLightPipeline.LoadShaders();
    uniformLightPipeline.RegisterScene(scene);
    uniformLightPipeline.RegisterGBuffers(gbufferPipeline.GetGBuffers());
    uniformLightPipeline.Setup(sizeof(PushConstants));

    wrsPipeline.LoadShaders();
    wrsPipeline.RegisterScene(scene);
    wrsPipeline.RegisterGBuffers(gbufferPipeline.GetGBuffers());
    wrsPipeline.Setup(sizeof(PushConstants));

    pushConstants.invProj = glm::inverse(scene.GetCamera().GetProj());
    pushConstants.invView = glm::inverse(scene.GetCamera().GetView());
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

        UI::StartFrame();
        UI::Combo("Method", method, { "Uniform", "WRS" });
        UI::Prepare();

        scene.Update(0.1);

        scene.ProcessInput();
        pushConstants.invProj = glm::inverse(scene.GetCamera().GetProj());
        pushConstants.invView = glm::inverse(scene.GetCamera().GetView());
        pushConstants.frame++;

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
