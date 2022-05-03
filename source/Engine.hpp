#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan_hpp.h>
#include <vulkan/vulkan.hpp>
#include "Image.hpp"
#include "Buffer.hpp"
#include "Accel.hpp"
#include "Pipeline.hpp"
#include "Window.hpp"

class Engine
{
public:
    static void Init();
    static void Shutdown();
    static void Run();

private:
    static inline Image renderImage;
    static inline ComputePipeline pipeline;
    static inline RayTracingPipeline rtPipeline;
    static inline Buffer vertexBuffer;
    static inline Buffer indexBuffer;
    static inline Accel bottomAccel;
    static inline Accel topAccel;
};
