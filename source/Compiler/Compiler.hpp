#pragma once
#include <string>
#include <vector>

namespace File {
std::string readFile(const std::string& path);
}

namespace Compiler {
vk::ShaderStageFlagBits getShaderStage(const std::string& filepath);

using Define = std::pair<std::string, std::string>;

void addDefines(std::string& glslCode, const std::vector<Define>& defines);

// Support include directive
std::vector<uint32_t> compileToSPV(const std::string& filepath,
                                   const std::vector<Define>& defines = {});

// Don't support include directive
// This is for hardcoded shader in C++
std::vector<uint32_t> compileToSPV(const std::string& glslCode,
                                   vk::ShaderStageFlagBits shaderStage,
                                   const std::vector<Define>& defines = {});
}  // namespace Compiler
