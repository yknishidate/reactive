#include <functional>
#include "Engine.hpp"
#include "Vulkan.hpp"
#include "Input.hpp"
#include "Loader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

struct BufferAddress
{
    vk::DeviceAddress vertices;
    vk::DeviceAddress indices;
    vk::DeviceAddress objects;
};

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
    //mesh = std::make_shared<Mesh>("../asset/viking_room/viking_room.obj");
    //mesh = std::make_shared<Mesh>("../asset/CornellBox.obj");
    //mesh = std::make_shared<Mesh>("../asset/crytek_sponza/sponza.obj");
    Loader::LoadFromFile("../asset/crytek_sponza/sponza.obj", meshes);
    texture.Init("../asset/viking_room/viking_room.png");

    objects.resize(meshes.size());
    for (int i = 0; i < meshes.size(); i++) {
        objects[i].Init(meshes[i]);
        objects[i].GetTransform().Position.y = 1.0;
        objects[i].GetTransform().Scale = glm::vec3{ 0.01 };
        objects[i].GetTransform().Rotation = glm::quat{ glm::vec3{ 0, glm::radians(90.0f), 0 } };
    }
    topAccel.InitAsTop(objects);

    // Create object data
    for (auto&& object : objects) {
        objectData.push_back({ object.GetTransform().GetMatrix(), object.GetTransform().GetNormalMatrix(), -1 });
    }
    objectBuffer.InitOnHost(sizeof(ObjectData) * objectData.size(),
                            vk::BufferUsageFlagBits::eStorageBuffer |
                            vk::BufferUsageFlagBits::eShaderDeviceAddress);
    objectBuffer.Copy(objectData.data());

    //BufferAddress address;
    //address.vertices = mesh->GetVertexBufferAddress();
    //address.indices = mesh->GetIndexBufferAddress();
    //address.objects = objectBuffer.GetAddress();
    //addressBuffer.InitOnHost(sizeof(BufferAddress), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress);
    //addressBuffer.Copy(&address);

    // Create pipelines
    rtPipeline.Init("../shader/pathtracing/pathtracing.rgen",
                    "../shader/pathtracing/pathtracing.rmiss",
                    "../shader/pathtracing/pathtracing.rchit", sizeof(PushConstants));
    rtPipeline.UpdateDescSet("inputImage", inputImage.GetView(), inputImage.GetSampler());
    rtPipeline.UpdateDescSet("outputImage", outputImage.GetView(), outputImage.GetSampler());
    rtPipeline.UpdateDescSet("topLevelAS", topAccel.GetAccel());
    rtPipeline.UpdateDescSet("samplers", texture.GetView(), texture.GetSampler());
    //rtPipeline.UpdateDescSet("Addresses", addressBuffer.GetBuffer(), addressBuffer.GetSize());

    medianPipeline.Init("../shader/denoise/median.comp", sizeof(PushConstants));
    medianPipeline.UpdateDescSet("inputImage", outputImage.GetView(), outputImage.GetSampler());
    medianPipeline.UpdateDescSet("outputImage", denoisedImage.GetView(), denoisedImage.GetSampler());

    // Create push constants
    camera.Init(Window::GetWidth(), Window::GetHeight());
    pushConstants.InvProj = glm::inverse(camera.GetProj());
    pushConstants.InvView = glm::inverse(camera.GetView());
    pushConstants.Frame = 0;
}

void Engine::Run()
{
    static bool accumulation = true;
    static int denoise = 0;
    spdlog::info("Engine::Run()");
    while (!Window::ShouldClose()) {
        Window::PollEvents();
        Input::Update();
        Window::StartFrame();

        ImGui::Checkbox("Accumulation", &accumulation);
        ImGui::Combo("Denoise", &denoise, "Off\0Median\0");
        ImGui::Render();

        // Update push constants
        camera.ProcessInput();
        pushConstants.InvProj = glm::inverse(camera.GetProj());
        pushConstants.InvView = glm::inverse(camera.GetView());
        if (!accumulation || camera.CheckDirtyAndClean()) {
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
