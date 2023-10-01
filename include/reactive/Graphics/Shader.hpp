#pragma once
#include "Compiler/Compiler.hpp"
#include "Context.hpp"

namespace rv {
struct ShaderCreateInfo {
    const std::vector<uint32_t>& code;
    vk::ShaderStageFlagBits stage;
};

class Shader {
public:
    Shader(const Context* context, ShaderCreateInfo createInfo);

    auto getSpvCode() const { return spvCode; }
    auto getModule() const { return *shaderModule; }
    auto getStage() const { return stage; }

private:
    vk::UniqueShaderModule shaderModule;
    vk::UniqueShaderEXT shader;
    std::vector<uint32_t> spvCode;
    vk::ShaderStageFlagBits stage;
};
}  // namespace rv
