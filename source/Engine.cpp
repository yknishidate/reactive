#include <functional>
#include "Engine.hpp"
#include "Vulkan/Vulkan.hpp"
#include "Input.hpp"
#include "Loader.hpp"
#include "Scene.hpp"
#include "Object.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace
{
    void CopyImages(vk::CommandBuffer commandBuffer, int width, int height,
                    vk::Image inputImage, vk::Image outputImage, vk::Image backImage)
    {
        // output -> input
        // output -> back
        vk::ImageCopy copyRegion;
        copyRegion.setSrcSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });
        copyRegion.setDstSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });
        copyRegion.setExtent({ uint32_t(width), uint32_t(height), 1 });

        Image::SetImageLayout(commandBuffer, outputImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal);
        Image::SetImageLayout(commandBuffer, inputImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferDstOptimal);
        Image::SetImageLayout(commandBuffer, backImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

        Image::CopyImage(commandBuffer, outputImage, inputImage, copyRegion);
        Image::CopyImage(commandBuffer, outputImage, backImage, copyRegion);

        Image::SetImageLayout(commandBuffer, outputImage, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eGeneral);
        Image::SetImageLayout(commandBuffer, inputImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral);
        Image::SetImageLayout(commandBuffer, backImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eColorAttachmentOptimal);
    }

    void CopyImages(vk::CommandBuffer commandBuffer, int width, int height,
                    vk::Image inputImage, vk::Image outputImage, vk::Image denoisedImage, vk::Image backImage)
    {
        // denoised -> back
        // output -> input
        vk::ImageCopy copyRegion;
        copyRegion.setSrcSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });
        copyRegion.setDstSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });
        copyRegion.setExtent({ uint32_t(width), uint32_t(height), 1 });

        Image::SetImageLayout(commandBuffer, denoisedImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal);
        Image::SetImageLayout(commandBuffer, outputImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal);
        Image::SetImageLayout(commandBuffer, inputImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferDstOptimal);
        Image::SetImageLayout(commandBuffer, backImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

        Image::CopyImage(commandBuffer, denoisedImage, backImage, copyRegion);
        Image::CopyImage(commandBuffer, outputImage, inputImage, copyRegion);

        Image::SetImageLayout(commandBuffer, denoisedImage, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eGeneral);
        Image::SetImageLayout(commandBuffer, outputImage, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eGeneral);
        Image::SetImageLayout(commandBuffer, inputImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral);
        Image::SetImageLayout(commandBuffer, backImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eColorAttachmentOptimal);
    }
}

void Engine::Init()
{
    spdlog::info("Engine::Init()");

    // Create resources
    inputImage.Init(Window::GetWidth(), Window::GetHeight(), vk::Format::eB8G8R8A8Unorm);
    outputImage.Init(Window::GetWidth(), Window::GetHeight(), vk::Format::eB8G8R8A8Unorm);
    denoisedImage.Init(Window::GetWidth(), Window::GetHeight(), vk::Format::eB8G8R8A8Unorm);

    // Load scene
    //scene = std::make_unique<Scene>("../asset/crytek_sponza/sponza.obj",
    //                                glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec3{ 0.01f },
    //                                glm::vec3{ 0.0f, glm::radians(90.0f), 0.0f });
    scene = std::make_unique<Scene>("../asset/CornellBox/CornellBox-Glossy.obj", glm::vec3{ 0.0f, 0.75f, 0.0f });

    // Add lights
//    scene->AddPointLight(glm::vec3{ 5.0f }, glm::vec3{ 0.0f });
    scene->AddSphereLight(glm::vec3{ 50.0f }, glm::vec3{ 0.0f, -0.75f, 0.0f }, 0.1f);
    scene->Setup();

    // Create pipelines
    rtPipeline.LoadShaders("../shader/pathtracing/pathtracing.rgen",
                           "../shader/pathtracing/pathtracing.rmiss",
                           "../shader/pathtracing/pathtracing.rchit");

    rtPipeline.Register("inputImage", inputImage);
    rtPipeline.Register("outputImage", outputImage);
    rtPipeline.Register("samplers", outputImage); // Dummy
    rtPipeline.Register("topLevelAS", scene->GetAccel());
    rtPipeline.Register("samplers", scene->GetTextures());
    rtPipeline.Register("Addresses", scene->GetAddressBuffer());
    rtPipeline.Setup(sizeof(PushConstants));

    medianPipeline.LoadShaders("../shader/denoise/median.comp");
    medianPipeline.Register("inputImage", outputImage);
    medianPipeline.Register("outputImage", denoisedImage);
    medianPipeline.Setup(sizeof(PushConstants));

    // Create push constants
    pushConstants.InvProj = glm::inverse(scene->GetCamera().GetProj());
    pushConstants.InvView = glm::inverse(scene->GetCamera().GetView());
    pushConstants.Frame = 0;
}

void Engine::Run()
{
    static bool accumulation = false;
    static bool importance = false;
    static bool nee = false;
    static int denoise = 0;
    spdlog::info("Engine::Run()");
    while (!Window::ShouldClose()) {
        Window::PollEvents();
        Input::Update();
        Window::StartFrame();

        ImGui::Checkbox("Accumulation", &accumulation);
        bool refresh = false;
        refresh |= ImGui::Checkbox("Importance sampling", &importance);
        refresh |= ImGui::Checkbox("NEE", &nee);
        ImGui::Combo("Denoise", &denoise, "Off\0Median\0");
        refresh |= ImGui::SliderInt("Depth", &pushConstants.Depth, 1, 8);
        refresh |= ImGui::SliderInt("Samples", &pushConstants.Samples, 1, 32);
        refresh |= ImGui::ColorPicker4("Sky color", pushConstants.SkyColor);
        ImGui::Render();

        // Scene update
        //scene->Update(0.1);

        // Update push constants
        scene->ProcessInput();
        pushConstants.InvProj = glm::inverse(scene->GetCamera().GetProj());
        pushConstants.InvView = glm::inverse(scene->GetCamera().GetView());
        pushConstants.Importance = static_cast<int>(importance);
        pushConstants.NEE = static_cast<int>(nee);
        if (refresh || !accumulation || scene->GetCamera().CheckDirtyAndClean()) {
            pushConstants.Frame = 0;
        } else {
            pushConstants.Frame++;
        }

        // Render
        if (!Window::IsMinimized()) {
            Window::WaitNextFrame();
            Window::BeginCommandBuffer();

            int width = Window::GetWidth();
            int height = Window::GetHeight();
            vk::CommandBuffer commandBuffer = Window::GetCurrentCommandBuffer();
            rtPipeline.Run(commandBuffer, width, height, &pushConstants);
            if (denoise) {
                medianPipeline.Run(commandBuffer, width, height, &pushConstants);
                CopyImages(commandBuffer, width, height, inputImage.GetImage(), outputImage.GetImage(), denoisedImage.GetImage(), Window::GetBackImage());
            } else {
                CopyImages(commandBuffer, width, height, inputImage.GetImage(), outputImage.GetImage(), Window::GetBackImage());
            }

            Window::RenderUI();
            Window::Submit();
            Window::Present();
        }
    }
    Vulkan::Device.waitIdle();
}
