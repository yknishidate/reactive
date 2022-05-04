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
    glm::mat4 invView;
    glm::mat4 invProj;
    int frame = 0;
};

struct ObjectData
{
    glm::mat4 Matrix;
    glm::mat4 NormalMatrix;
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
    std::vector<Object> objects;

    std::vector<ObjectData> objectData;
    Buffer objectBuffer;

    std::shared_ptr<Mesh> mesh;
    Camera camera;
    Accel topAccel;
    PushConstants pushConstants;
    Buffer addressBuffer;
};
