#pragma once
#include <filesystem>
#include <unordered_map>
#include <spirv_glsl.hpp>
#include "Buffer.hpp"
#include "Image.hpp"
#include "Accel.hpp"
#include "Compiler/Compiler.hpp"

class Shader
{
public:
    Shader(const std::string& filepath);

    auto getSpvCode() const { return spvCode; }
    auto getModule() const { return *shaderModule; }
    auto getStage() const { return shaderStage; }

private:
    vk::UniqueShaderModule shaderModule;
    std::vector<uint32_t> spvCode;
    vk::ShaderStageFlags shaderStage;
};
