#include "Graphics/Shader.hpp"

namespace rv {
Shader::Shader(const Context* context, ShaderCreateInfo createInfo)
    : spvCode(createInfo.code), stage(createInfo.stage) {
    shaderModule = context->getDevice().createShaderModuleUnique(
        vk::ShaderModuleCreateInfo().setCode(spvCode));
}
}  // namespace rv
