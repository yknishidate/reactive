#include "Shader.hpp"

Shader::Shader(const Context* context, ShaderCreateInfo createInfo)
    : shaderStage{createInfo.shaderStage} {
    spvCode = Compiler::compileToSPV(createInfo.glslCode, shaderStage);
    shaderModule = context->getDevice().createShaderModuleUnique(
        vk::ShaderModuleCreateInfo().setCode(spvCode));
}

Shader::Shader(const Context* context, const std::string& filepath) {
    spvCode = Compiler::compileToSPV(filepath);

    shaderModule = context->getDevice().createShaderModuleUnique(
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
    else if (filepath.ends_with("mesh"))
        shaderStage = vk::ShaderStageFlagBits::eMeshEXT;
    else if (filepath.ends_with("task"))
        shaderStage = vk::ShaderStageFlagBits::eTaskEXT;
    else
        assert(false && "Unknown shader stage");
}

Shader::Shader(const Context* context,
               const std::string& glslCode,
               vk::ShaderStageFlagBits shaderStage)
    : shaderStage{shaderStage} {
    spvCode = Compiler::compileToSPV(glslCode, shaderStage);
    shaderModule = context->getDevice().createShaderModuleUnique(
        vk::ShaderModuleCreateInfo().setCode(spvCode));
}

Shader::Shader(const Context* context,
               const std::vector<uint32_t>& spvCode,
               vk::ShaderStageFlagBits shaderStage)
    : spvCode{spvCode}, shaderStage{shaderStage} {
    shaderModule = context->getDevice().createShaderModuleUnique(
        vk::ShaderModuleCreateInfo().setCode(spvCode));
}
