#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan_hpp.h>
#include <vulkan/vulkan.hpp>

#include "Image.hpp"
#include "Mesh.hpp"
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
    static void Init();
    static void Shutdown();
    static void Run();

private:
    static inline Image renderImage;
    static inline Image texture;
    static inline ComputePipeline pipeline;
    static inline RayTracingPipeline rtPipeline;
    static inline Mesh mesh;
    static inline Camera camera;
    static inline Accel topAccel;
    static inline PushConstants pushConstants;
    static inline Buffer addressBuffer;
};
