#include "Shader.hpp"

Shader::Shader(const std::string& filepath) {
    spvCode = Compiler::compileToSPV(filepath);

    shaderModule = Context::getDevice().createShaderModuleUnique(
        vk::ShaderModuleCreateInfo().setCode(spvCode));

    if (filepath.ends_with("vert"))
        shaderStage = vk::ShaderStageFlagBits::eVertex;
    else if (filepath.ends_with("frag"))
        shaderStage = vk::ShaderStageFlagBits::eFragment;
    else if (filepath.ends_with("comp"))
        shaderStage = vk::ShaderStageFlagBits::eCompute;
    else if (filepath.ends_with("rgen"))
        shaderStage = vk::ShaderStageFlagBits::eRaygenKHR;
    else if (filepath.ends_with("rmiss"))
        shaderStage = vk::ShaderStageFlagBits::eMissKHR;
    else if (filepath.ends_with("rchit"))
        shaderStage = vk::ShaderStageFlagBits::eClosestHitKHR;
    else if (filepath.ends_with("rahit"))
        shaderStage = vk::ShaderStageFlagBits::eAnyHitKHR;
    else
        assert(false && "Unknown shader stage");
}

Shader::Shader(const std::string& glslCode, vk::ShaderStageFlagBits shaderStage) {
    this->shaderStage = shaderStage;
    spvCode = Compiler::compileToSPV(glslCode, shaderStage);
    shaderModule = Context::getDevice().createShaderModuleUnique(
        vk::ShaderModuleCreateInfo().setCode(spvCode));
}
