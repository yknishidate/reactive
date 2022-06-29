#pragma once
#include <memory>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>

#include "ImGui/imgui_impl_vulkan_hpp.h"
#include "Vulkan/Image.hpp"
#include "Vulkan/Pipeline.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Window.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"

class Engine
{
public:
    void Init();
    void Run();

private:
    Image inputImage;
    Image outputImage;
    Image denoisedImage;
    Image reservoirSampleImage;
    Image reservoirWeightImage;
    Image posImage;
    Image normalImage;
    Image indexImage;
    Image diffuseImage;
    Image emissionImage;
    //DescriptorSet descSet;
    ComputePipeline meanPipeline;
    ComputePipeline medianPipeline;
    RayTracingPipeline ptPipeline;
    RayTracingPipeline neePipeline;
    RayTracingPipeline srisPipeline;
    RayTracingPipeline initReservoirPipeline;
    RayTracingPipeline shadingPipeline;
    PushConstants pushConstants;
    std::unique_ptr<Scene> scene;
};
