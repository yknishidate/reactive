#pragma once
#include <string>
#include <vector>

namespace File {
std::string readFile(const std::string& path);
}

namespace Compiler {
vk::ShaderStageFlagBits getShaderStage(const std::string& filepath);
std::vector<uint32_t> compileToSPV(const std::string& filepath);
std::vector<uint32_t> compileToSPV(const std::string& glslCode,
                                   vk::ShaderStageFlagBits shaderStage);
}  // namespace Compiler
