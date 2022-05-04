#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan_hpp.h>
#include <vulkan/vulkan.hpp>

#include "Image.hpp"
#include "Object.hpp"
#include "Pipeline.hpp"
#include "Window.hpp"
#include "Camera.hpp"

struct PushConstants
{
    glm::mat4 invView;
    glm::mat4 invProj;
    int frame = 0;
};

class Engine
{
public:
    void Init();
    void Run();

private:
    Image inputImage;
    Image outputImage;
    Image texture;
    RayTracingPipeline rtPipeline;
    Object object;
    std::shared_ptr<Mesh> mesh;
    Camera camera;
    Accel topAccel;
    PushConstants pushConstants;
    Buffer addressBuffer;
};
