#pragma once
#include <string>
#include <vector>

namespace rv {
namespace File {
std::string readFile(const std::filesystem::path& path);

std::filesystem::file_time_type getLastWriteTimeWithIncludeFiles(
    const std::filesystem::path& filepath);

template <typename T>
void writeBinary(const std::filesystem::path& filepath, const std::vector<T>& vec) {
    std::ofstream ofs(filepath, std::ios::out | std::ios::binary);
    if (!ofs) {
        throw std::runtime_error("File::writeBinary | Failed to open file: " + filepath.string());
    }
    ofs.write(reinterpret_cast<const char*>(vec.data()), vec.size() * sizeof(T));
    ofs.close();
}

template <typename T>
void readBinary(const std::filesystem::path& filepath, std::vector<T>& vec) {
    std::uintmax_t size = std::filesystem::file_size(filepath);
    vec.resize(size / sizeof(T));
    std::ifstream ifs(filepath, std::ios::in | std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("File::readBinary | Failed to open file: " + filepath.string());
    }
    ifs.read(reinterpret_cast<char*>(vec.data()), vec.size() * sizeof(T));
    ifs.close();
}
}  // namespace File

enum class ShaderStage {
    Vertex,
    Fragment,
    Compute,
    Raygen,
    AnyHit,
    ClosestHit,
    Miss,
    Intersection,
    Callable,
    Task,
    Mesh,
};

namespace Compiler {
ShaderStage getShaderStage(const std::string& filepath);

using Define = std::pair<std::string, std::string>;

void addDefines(std::string& glslCode, const std::vector<Define>& defines);

// This supports include directive
std::vector<uint32_t> compileToSPV(const std::string& filepath,
                                   const std::vector<Define>& defines = {});

// This doesn't support include directive
// This is for hardcoded shader in C++
std::vector<uint32_t> compileToSPV(const std::string& glslCode,
                                   ShaderStage shaderStage,
                                   const std::vector<Define>& defines = {});

std::vector<std::string> getAllIncludedFiles(const std::string& code);
}  // namespace Compiler
}  // namespace rv
