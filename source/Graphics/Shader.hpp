#pragma once
#include <filesystem>
#include <spirv_glsl.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include "Accel.hpp"
#include "Buffer.hpp"
#include "Compiler/Compiler.hpp"
#include "Context.hpp"
#include "Image.hpp"

struct ShaderCreateInfo {
    const std::string& glslCode;
    vk::ShaderStageFlagBits stage;
};

class Shader {
public:
    Shader() = default;
    Shader(const Context* context, ShaderCreateInfo createInfo);

    auto getSpvCode() const { return spvCode; }
    auto getModule() const { return *shaderModule; }
    auto getStage() const { return shaderStage; }

private:
    vk::UniqueShaderModule shaderModule;
    vk::UniqueShaderEXT shader;
    std::vector<uint32_t> spvCode;
    vk::ShaderStageFlagBits shaderStage;
};
