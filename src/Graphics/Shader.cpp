#include "reactive/Graphics/Shader.hpp"

namespace rv {
Shader::Shader(const Context& context, const ShaderCreateInfo& createInfo)
    : m_pCode(createInfo.pCode), m_codeSize(createInfo.codeSize), m_stage(createInfo.stage) {
    vk::ShaderModuleCreateInfo moduleInfo;
    moduleInfo.setPCode(reinterpret_cast<const uint32_t*>(m_pCode));
    moduleInfo.setCodeSize(m_codeSize);
    m_shaderModule = context.getDevice().createShaderModuleUnique(moduleInfo);
}
}  // namespace rv
