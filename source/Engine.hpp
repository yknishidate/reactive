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
    static void Init();
    static void Shutdown();
    static void Run();

private:
    static inline Image inputImage;
    static inline Image outputImage;
    static inline Image texture;
    static inline RayTracingPipeline rtPipeline;
    static inline Object object;
    static inline std::shared_ptr<Mesh> mesh;
    static inline Camera camera;
    static inline Accel topAccel;
    static inline PushConstants pushConstants;
    static inline Buffer addressBuffer;
};
