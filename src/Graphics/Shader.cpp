#include "reactive/Graphics/Shader.hpp"

namespace rv {
Shader::Shader(const Context& context, const ShaderCreateInfo& createInfo)
    : pCode(createInfo.pCode), codeSize(createInfo.codeSize), stage(createInfo.stage) {
    vk::ShaderModuleCreateInfo moduleInfo;
    moduleInfo.setPCode(reinterpret_cast<const uint32_t*>(pCode));
    moduleInfo.setCodeSize(codeSize);
    shaderModule = context.getDevice().createShaderModuleUnique(moduleInfo);
}
}  // namespace rv
