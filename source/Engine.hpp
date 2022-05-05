#pragma once
#include "Vulkan.hpp"
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan_hpp.h>

#include "Image.hpp"
#include "Object.hpp"
#include "Pipeline.hpp"
#include "Window.hpp"
#include "Camera.hpp"

struct PushConstants
{
    glm::mat4 InvView{ 1 };
    glm::mat4 InvProj{ 1 };
    int Frame = 0;
};

struct ObjectData
{
    glm::mat4 Matrix{ 1 };
    glm::mat4 NormalMatrix{ 1 };
    int TextureIndex = -1;
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
    Image texture;
    ComputePipeline meanPipeline;
    ComputePipeline medianPipeline;
    RayTracingPipeline rtPipeline;
    std::vector<Object> objects;

    std::vector<ObjectData> objectData;
    Buffer objectBuffer;

    std::vector<std::shared_ptr<Mesh>> meshes;
    Camera camera;
    Accel topAccel;
    PushConstants pushConstants;
    Buffer addressBuffer;
};
