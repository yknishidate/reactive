#include "reactive/Graphics/Shader.hpp"

namespace rv {
Shader::Shader(const Context& context, const ShaderCreateInfo& createInfo)
    : spvCode(createInfo.code), stage(createInfo.stage) {
    vk::ShaderModuleCreateInfo moduleInfo;
    moduleInfo.setCode(spvCode);
    shaderModule = context.getDevice().createShaderModuleUnique(moduleInfo);
}
}  // namespace rv
