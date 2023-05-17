#include "Shader.hpp"

Shader::Shader(const Context* context, ShaderCreateInfo createInfo)
    : shaderStage{createInfo.stage} {
    spvCode = Compiler::compileToSPV(createInfo.glslCode, shaderStage);
    shaderModule = context->getDevice().createShaderModuleUnique(
        vk::ShaderModuleCreateInfo().setCode(spvCode));
}
