#pragma once
#include "Context.hpp"

namespace rv {
struct ShaderCreateInfo {
    const void* pCode;
    const size_t codeSize;
    vk::ShaderStageFlagBits stage;
};

class Shader {
public:
    Shader(const Context& context, const ShaderCreateInfo& createInfo);

    auto getSpvCodePtr() const { return m_pCode; }
    auto getSpvCodeSize() const { return m_codeSize; }
    auto getModule() const { return *m_shaderModule; }
    auto getStage() const { return m_stage; }

private:
    vk::UniqueShaderModule m_shaderModule;
    vk::UniqueShaderEXT m_shader;
    const void* m_pCode;
    const size_t m_codeSize;
    vk::ShaderStageFlagBits m_stage;
};
}  // namespace rv
