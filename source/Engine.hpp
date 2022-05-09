#pragma once
#include <memory>
#include "Vulkan/Vulkan.hpp"
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan_hpp.h>

#include "Vulkan/Image.hpp"
#include "Vulkan/Pipeline.hpp"
#include "Window.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"

struct PushConstants
{
    glm::mat4 InvView{ 1 };
    glm::mat4 InvProj{ 1 };
    int Frame = 0;
    int Importance = 0;
    int Depth = 4;
    int Samples = 1;
    float SkyColor[4] = { 1, 1, 1, 0 };
};

class Engine
{
public:
    void Init();
    void Run();

private:
    Image inputImage;
    Image outputImage;
    Image denoisedImage;
    ComputePipeline meanPipeline;
    ComputePipeline medianPipeline;
    RayTracingPipeline rtPipeline;
    PushConstants pushConstants;
    std::unique_ptr<Scene> scene;
};
