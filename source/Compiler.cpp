#include <regex>
#include <fstream>
#include <filesystem>

#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/ResourceLimits.h>
#include <spirv_glsl.hpp>
#include <spdlog/spdlog.h>
#include "Compiler.hpp"

namespace Compiler
{
    std::string ReadFile(const std::string& path)
    {
        std::ifstream input_file{ path };
        if (!input_file.is_open()) {
            throw std::runtime_error("Failed to open file: " + path);
        }
        return { (std::istreambuf_iterator<char>{input_file}), std::istreambuf_iterator<char>{} };
    }

    EShLanguage GetShaderStage(const std::string& filepath)
    {
        if (filepath.ends_with("vert")) return EShLangVertex;
        else if (filepath.ends_with("frag")) return EShLangFragment;
        else if (filepath.ends_with("comp")) return EShLangCompute;
        else if (filepath.ends_with("rgen")) return EShLangRayGenNV;
        else if (filepath.ends_with("rmiss")) return EShLangMissNV;
        else if (filepath.ends_with("rchit")) return EShLangClosestHitNV;
        else if (filepath.ends_with("rahit")) return EShLangAnyHitNV;
        else assert(false && "Unknown shader stage");
    }

    std::string Include(const std::string& filepath, const std::string& sourceText)
    {
        using std::filesystem::path;
        path dir = path{ filepath }.parent_path();

        std::string included = sourceText;
        std::regex regex{ "#include \"(.*)\"" };
        std::smatch results;
        while (std::regex_search(included, results, regex)) {
            path includePath = dir / results[1].str();
            std::string includeText = ReadFile(includePath.string());
            included.replace(results.position(), results.length(), includeText);
        }
        return included;
    }

    std::vector<uint32_t> CompileToSPV(const std::string& filepath)
    {
        spdlog::info("  Compile shader: {}", filepath);
        std::string glslShader = ReadFile(filepath);
        EShLanguage stage = GetShaderStage(filepath);
        std::string included = Include(filepath, glslShader);

        glslang::InitializeProcess();

        const char* shaderStrings = included.data();
        glslang::TShader shader(stage);
        shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_4);
        shader.setStrings(&shaderStrings, 1);

        EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
        if (!shader.parse(&glslang::DefaultTBuiltInResource, 100, false, messages)) {
            throw std::runtime_error("Failed to parse: " + glslShader + "\n" + shader.getInfoLog());
        }

        glslang::TProgram program;
        program.addShader(&shader);
        if (!program.link(messages)) {
            throw std::runtime_error("Failed to link: " + glslShader + "\n" + shader.getInfoLog());
        }

        std::vector<uint32_t> spvShader;
        glslang::GlslangToSpv(*program.getIntermediate(stage), spvShader);
        glslang::FinalizeProcess();
        return spvShader;
    }
} // namespace
