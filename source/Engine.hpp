#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan_hpp.h>
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "Image.hpp"
#include "Buffer.hpp"
#include "Accel.hpp"
#include "Pipeline.hpp"
#include "Window.hpp"

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
    static inline ComputePipeline pipeline;
    static inline RayTracingPipeline rtPipeline;
    static inline Buffer vertexBuffer;
    static inline Buffer indexBuffer;
    static inline Accel bottomAccel;
    static inline Accel topAccel;
    static inline PushConstants pushConstants;
};
