#pragma once
#include <vector>
#include <string>

namespace Compiler
{
std::vector<uint32_t> compileToSPV(const std::string& filepath);
std::vector<uint32_t> compileToSPV(const std::string& glslCode, vk::ShaderStageFlagBits shaderStage);
}
