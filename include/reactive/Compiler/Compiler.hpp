#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include <slang/slang.h>
#include <slang/slang-com-ptr.h>

namespace rv {

namespace File {
auto readFile(const std::filesystem::path& path) -> std::string;

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

class SlangCompiler
{
public:
    SlangCompiler();

    std::vector<Slang::ComPtr<slang::IBlob>> compileShaders(
        const std::filesystem::path& shaderPath,
        const std::vector<std::string>& entryPointNames);

private:
    Slang::ComPtr<slang::IGlobalSession> m_globalSession;
};

}  // namespace rv
