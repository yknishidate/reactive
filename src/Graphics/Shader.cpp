#include "Graphics/Shader.hpp"

namespace rv {
Shader::Shader(const Context* context, ShaderCreateInfo createInfo)
    : shaderStage{createInfo.stage}, spvCode(createInfo.code) {
    shaderModule = context->getDevice().createShaderModuleUnique(
        vk::ShaderModuleCreateInfo().setCode(spvCode));
}
}  // namespace rv
