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

    auto getSpvCodePtr() const { return pCode; }
    auto getSpvCodeSize() const { return codeSize; }
    auto getModule() const { return *shaderModule; }
    auto getStage() const { return stage; }

private:
    vk::UniqueShaderModule shaderModule;
    vk::UniqueShaderEXT shader;
    const void* pCode;
    const size_t codeSize;
    vk::ShaderStageFlagBits stage;
};
}  // namespace rv
