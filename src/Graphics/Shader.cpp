#include "Graphics/Shader.hpp"

namespace {
vk::ShaderStageFlagBits getShaderStage(rv::ShaderStage stage) {
    switch (stage) {
        case rv::ShaderStage::Vertex:
            return vk::ShaderStageFlagBits::eVertex;
        case rv::ShaderStage::Fragment:
            return vk::ShaderStageFlagBits::eFragment;
        case rv::ShaderStage::Compute:
            return vk::ShaderStageFlagBits::eCompute;
        case rv::ShaderStage::Raygen:
            return vk::ShaderStageFlagBits::eRaygenKHR;
        case rv::ShaderStage::AnyHit:
            return vk::ShaderStageFlagBits::eAnyHitKHR;
        case rv::ShaderStage::ClosestHit:
            return vk::ShaderStageFlagBits::eClosestHitKHR;
        case rv::ShaderStage::Miss:
            return vk::ShaderStageFlagBits::eMissKHR;
        case rv::ShaderStage::Intersection:
            return vk::ShaderStageFlagBits::eIntersectionKHR;
        case rv::ShaderStage::Callable:
            return vk::ShaderStageFlagBits::eCallableKHR;
        case rv::ShaderStage::Task:
            return vk::ShaderStageFlagBits::eTaskEXT;
        case rv::ShaderStage::Mesh:
            return vk::ShaderStageFlagBits::eMeshEXT;
    }
}
}  // namespace

namespace rv {
Shader::Shader(const Context* context, ShaderCreateInfo createInfo) : spvCode(createInfo.code) {
    shaderModule = context->getDevice().createShaderModuleUnique(
        vk::ShaderModuleCreateInfo().setCode(spvCode));
    stage = getShaderStage(createInfo.stage);
}
}  // namespace rv
