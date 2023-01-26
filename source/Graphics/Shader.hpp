#pragma once
#include <filesystem>
#include <spirv_glsl.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include "Accel.hpp"
#include "Buffer.hpp"
#include "Compiler/Compiler.hpp"
#include "Image.hpp"

class Shader {
public:
    Shader() = default;
    Shader(const std::string& filepath);
    Shader(const std::string& glslCode, vk::ShaderStageFlagBits shaderStage);
    Shader(const std::vector<uint32_t>& spvCode, vk::ShaderStageFlagBits shaderStage);

    auto getSpvCode() const { return spvCode; }
    auto getModule() const { return *shaderModule; }
    auto getStage() const { return shaderStage; }

private:
    vk::UniqueShaderModule shaderModule;
    std::vector<uint32_t> spvCode;
    vk::ShaderStageFlagBits shaderStage;
};
