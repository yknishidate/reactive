#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include <slang/slang.h>
#include <slang/slang-com-ptr.h>

using Slang::ComPtr;

#define ASSERT_ON_SLANG_FAIL(x)               \
    {                                         \
        SlangResult _res = (x);               \
        assert(SLANG_SUCCEEDED(_res)); \
    }

namespace rv {

inline void diagnoseIfNeeded(slang::IBlob* diagnosticsBlob) {
    if (diagnosticsBlob != nullptr) {
        spdlog::error((const char*)diagnosticsBlob->getBufferPointer());
    }
}

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

    SlangCompiler()
    {
        // First we need to create slang global session with work with the Slang API.
        ASSERT_ON_SLANG_FAIL(slang::createGlobalSession(slangGlobalSession.writeRef()));
    }


    std::vector<ComPtr<slang::IBlob>> CompileShaders(const std::filesystem::path& shaderPath,
                 const std::string& vertexEntryPointName,
                 const std::string& fragmentEntryPointName)
    {
        assert(slangGlobalSession);

        // Next we create a compilation session to generate SPIRV code from Slang source.
        slang::SessionDesc sessionDesc = {};
        slang::TargetDesc targetDesc = {};
        targetDesc.format = SLANG_SPIRV;
        targetDesc.profile = slangGlobalSession->findProfile("spirv_1_5");
        targetDesc.flags = 0;

        sessionDesc.targets = &targetDesc;
        sessionDesc.targetCount = 1;

        std::vector<slang::CompilerOptionEntry> options;
        options.push_back({slang::CompilerOptionName::EmitSpirvDirectly,
                           {slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}});
        sessionDesc.compilerOptionEntries = options.data();
        sessionDesc.compilerOptionEntryCount = (uint32_t)options.size();

        Slang::ComPtr<slang::ISession> session;
        ASSERT_ON_SLANG_FAIL(slangGlobalSession->createSession(sessionDesc, session.writeRef()));

        Slang::ComPtr<slang::IBlob> diagnosticBlob;
        slang::IModule* slangModule = nullptr;
        {
            slangModule =
                session->loadModule(shaderPath.string().c_str(), diagnosticBlob.writeRef());
            diagnoseIfNeeded(diagnosticBlob);
            if (!slangModule) {
                return {};
            }
        }

        ComPtr<slang::IEntryPoint> vertexEntryPoint;
        ASSERT_ON_SLANG_FAIL(slangModule->findEntryPointByName(vertexEntryPointName.c_str(),
                                                               vertexEntryPoint.writeRef()));

        ComPtr<slang::IEntryPoint> fragmentEntryPoint;
        ASSERT_ON_SLANG_FAIL(slangModule->findEntryPointByName(fragmentEntryPointName.c_str(),
                                                               fragmentEntryPoint.writeRef()));

        std::vector<slang::IComponentType*> componentTypes;
        componentTypes.push_back(slangModule);

        int entryPointCount = 0;
        [[maybe_unused]] int vertexEntryPointIndex = entryPointCount++;
        componentTypes.push_back(vertexEntryPoint);

        [[maybe_unused]] int fragmentEntryPointIndex = entryPointCount++;
        componentTypes.push_back(fragmentEntryPoint);

        ComPtr<slang::IComponentType> linkedProgram;
        SlangResult result = session->createCompositeComponentType(
            componentTypes.data(), componentTypes.size(), linkedProgram.writeRef(),
            diagnosticBlob.writeRef());
        diagnoseIfNeeded(diagnosticBlob);
        ASSERT_ON_SLANG_FAIL(result);

        std::vector<ComPtr<slang::IBlob>> codes(2);
         //vertexSpirvCode;
        //ComPtr<slang::IBlob> fragmentSpirvCode;
        {
            result = linkedProgram->getEntryPointCode(vertexEntryPointIndex, 0, codes[0].writeRef(),
                                                      diagnosticBlob.writeRef());
            diagnoseIfNeeded(diagnosticBlob);
            ASSERT_ON_SLANG_FAIL(result);

            result = linkedProgram->getEntryPointCode(
                fragmentEntryPointIndex, 0, codes[1].writeRef(),
                                                      diagnosticBlob.writeRef());
            diagnoseIfNeeded(diagnosticBlob);
            ASSERT_ON_SLANG_FAIL(result);

            // if (isTestMode()) {
            //     printEntrypointHashes(1, 1, composedProgram);
            // }
        }
        return codes;
    }

private:
    Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;
};

namespace Compiler {
auto getLastWriteTimeWithIncludeFiles(const std::filesystem::path& filepath)
    -> std::filesystem::file_time_type;

auto shouldRecompile(const std::filesystem::path& glslFilepath,
                     const std::filesystem::path& spvFilepath) -> bool;

auto compileOrReadShader(const std::filesystem::path& glslFilepath,
                         const std::filesystem::path& spvFilepath) -> std::vector<uint32_t>;

auto getShaderStage(const std::string& filepath) -> vk::ShaderStageFlagBits;

using Define = std::pair<std::string, std::string>;

void addDefines(std::string& glslCode, const std::vector<Define>& defines);

// This supports include directive
auto compileToSPV(const std::filesystem::path& filepath, const std::vector<Define>& defines = {})
    -> std::vector<uint32_t>;

// This doesn't support include directive
// This is for hardcoded shader in C++
auto compileToSPV(const std::string& glslCode,
                  vk::ShaderStageFlagBits shaderStage,
                  const std::vector<Define>& defines = {}) -> std::vector<uint32_t>;

auto getAllIncludedFiles(const std::string& code) -> std::vector<std::string>;
}  // namespace Compiler
}  // namespace rv
