#pragma once
#include <string>
#include <vector>

namespace Compiler {
std::vector<uint32_t> compileToSPV(const std::string& filepath);
std::vector<uint32_t> compileToSPV(const std::string& glslCode,
                                   vk::ShaderStageFlagBits shaderStage);
}  // namespace Compiler
