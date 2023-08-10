#include "Compiler/Compiler.hpp"
#include <GlslangToSpv.h>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>
#include <regex>

namespace rv {
namespace File {
std::string readFile(const std::filesystem::path& path) {
    std::ifstream input_file{path};
    if (!input_file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path.string());
    }
    return {(std::istreambuf_iterator<char>{input_file}), std::istreambuf_iterator<char>{}};
}

std::filesystem::file_time_type getLastWriteTimeWithIncludeFiles(
    const std::filesystem::path& filepath) {
    auto directory = filepath.parent_path();
    auto writeTime = std::filesystem::last_write_time(filepath);
    std::string code = File::readFile(filepath);
    for (auto& include : Compiler::getAllIncludedFiles(code)) {
        auto includeWriteTime = getLastWriteTimeWithIncludeFiles(directory / include);
        if (includeWriteTime > writeTime) {
            writeTime = includeWriteTime;
        }
    }
    return writeTime;
}
}  // namespace File

namespace Compiler {
const TBuiltInResource DefaultTBuiltInResource = {
    /* .MaxLights = */ 32,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ 65535,
    /* .MaxComputeWorkGroupCountY = */ 65535,
    /* .MaxComputeWorkGroupCountZ = */ 65535,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .maxMeshOutputVerticesNV = */ 256,
    /* .maxMeshOutputPrimitivesNV = */ 512,
    /* .maxMeshWorkGroupSizeX_NV = */ 32,
    /* .maxMeshWorkGroupSizeY_NV = */ 1,
    /* .maxMeshWorkGroupSizeZ_NV = */ 1,
    /* .maxTaskWorkGroupSizeX_NV = */ 32,
    /* .maxTaskWorkGroupSizeY_NV = */ 1,
    /* .maxTaskWorkGroupSizeZ_NV = */ 1,
    /* .maxMeshViewCountNV = */ 4,
    /* .maxMeshOutputVerticesEXT = */ 256,
    /* .maxMeshOutputPrimitivesEXT = */ 256,
    /* .maxMeshWorkGroupSizeX_EXT = */ 128,
    /* .maxMeshWorkGroupSizeY_EXT = */ 128,
    /* .maxMeshWorkGroupSizeZ_EXT = */ 128,
    /* .maxTaskWorkGroupSizeX_EXT = */ 128,
    /* .maxTaskWorkGroupSizeY_EXT = */ 128,
    /* .maxTaskWorkGroupSizeZ_EXT = */ 128,
    /* .maxMeshViewCountEXT = */ 4,
    /* .maxDualSourceDrawBuffersEXT = */ 1,

    /* .limits = */
    {
        /* .nonInductiveForLoops = */ 1,
        /* .whileLoops = */ 1,
        /* .doWhileLoops = */ 1,
        /* .generalUniformIndexing = */ 1,
        /* .generalAttributeMatrixVectorIndexing = */ 1,
        /* .generalVaryingIndexing = */ 1,
        /* .generalSamplerIndexing = */ 1,
        /* .generalVariableIndexing = */ 1,
        /* .generalConstantMatrixVectorIndexing = */ 1,
    }};

vk::ShaderStageFlagBits getShaderStage(const std::string& filepath) {
    if (filepath.ends_with("vert"))
        return vk::ShaderStageFlagBits::eVertex;
    if (filepath.ends_with("frag"))
        return vk::ShaderStageFlagBits::eFragment;
    if (filepath.ends_with("comp"))
        return vk::ShaderStageFlagBits::eCompute;
    if (filepath.ends_with("rgen"))
        return vk::ShaderStageFlagBits::eRaygenKHR;
    if (filepath.ends_with("rmiss"))
        return vk::ShaderStageFlagBits::eMissKHR;
    if (filepath.ends_with("rchit"))
        return vk::ShaderStageFlagBits::eClosestHitKHR;
    if (filepath.ends_with("rahit"))
        return vk::ShaderStageFlagBits::eAnyHitKHR;
    if (filepath.ends_with("mesh"))
        return vk::ShaderStageFlagBits::eMeshEXT;
    if (filepath.ends_with("task"))
        return vk::ShaderStageFlagBits::eTaskEXT;
    assert(false && "Unknown shader stage");
}

EShLanguage translateShaderStage(vk::ShaderStageFlagBits shaderStage) {
    if (shaderStage == vk::ShaderStageFlagBits::eVertex)
        return EShLangVertex;
    if (shaderStage == vk::ShaderStageFlagBits::eFragment)
        return EShLangFragment;
    if (shaderStage == vk::ShaderStageFlagBits::eCompute)
        return EShLangCompute;
    if (shaderStage == vk::ShaderStageFlagBits::eRaygenKHR)
        return EShLangRayGenNV;
    if (shaderStage == vk::ShaderStageFlagBits::eMissKHR)
        return EShLangMissNV;
    if (shaderStage == vk::ShaderStageFlagBits::eClosestHitKHR)
        return EShLangClosestHitNV;
    if (shaderStage == vk::ShaderStageFlagBits::eAnyHitKHR)
        return EShLangAnyHitNV;
    if (shaderStage == vk::ShaderStageFlagBits::eMeshNV)
        return EShLangMesh;
    if (shaderStage == vk::ShaderStageFlagBits::eTaskEXT)
        return EShLangTask;
    assert(false && "Unknown shader stage");
}

std::string include(const std::string& filepath, const std::string& sourceText) {
    std::filesystem::path dir = std::filesystem::path{filepath}.parent_path();

    std::string included = sourceText;
    std::regex regex{"#include \"(.*)\""};
    std::smatch results;
    while (std::regex_search(included, results, regex)) {
        std::string includePath = dir.string() + "/" + results[1].str();
        std::string includeText = File::readFile(includePath);
        included.replace(results.position(), results.length(), includeText);
    }
    return included;
}

// NOTE: Define = std::pair<std::string, std::string>
void addDefines(std::string& glslCode, const std::vector<Define>& defines) {
    auto versionPos = glslCode.find("#version");
    if (versionPos == std::string::npos) {
        throw std::runtime_error("Shader has no #version");
    }

    // NOTE: initial offset is next line position of the #version directive
    size_t offset = glslCode.find('\n', versionPos) + 1;
    for (const auto& [name, value] : defines) {
        std::string defineString = "#define ";  // "#define "
        defineString.append(name);              // "#define HOGE"
        defineString.append(" ");               // "#define HOGE "
        defineString.append(value);             // "#define HOGE 0"
        defineString.append("\n");              // "#define HOGE 0\n"
        glslCode.insert(offset, defineString);
        offset += defineString.size();
    }
}

std::string addLineNumbersToCode(const std::string& code) {
    std::string addedCode;
    int lineIndex = 1;
    std::string line;
    std::istringstream iss(code);
    while (std::getline(iss, line)) {
        addedCode += std::to_string(lineIndex) + ": " + line + "\n";
        lineIndex++;
    }
    return addedCode;
}

// Main compile function
std::vector<uint32_t> compileToSPV(const std::string& glslCode, EShLanguage stage) {
    glslang::InitializeProcess();

    const char* shaderStrings = glslCode.data();
    glslang::TShader shader(stage);
    shader.setEnvClient(glslang::EShClient::EShClientVulkan,
                        glslang::EShTargetClientVersion::EShTargetVulkan_1_2);
    shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv,
                        glslang::EShTargetLanguageVersion::EShTargetSpv_1_4);
    shader.setStrings(&shaderStrings, 1);

    auto messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
    if (!shader.parse(&DefaultTBuiltInResource, 100, false, messages)) {
        std::string message = "Failed to parse:";
        message += addLineNumbersToCode(glslCode);
        message += "\n" + std::string(shader.getInfoLog());
        message += "\n" + std::string(shader.getInfoDebugLog());
        throw std::runtime_error(message);
    }

    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(messages)) {
        // throw std::runtime_error("Failed to link:\n" + glslCode + "\n" + shader.getInfoLog());
        std::string message = "Failed to link:";
        message += addLineNumbersToCode(glslCode);
        message += "\n" + std::string(shader.getInfoLog());
        message += "\n" + std::string(shader.getInfoDebugLog());
        throw std::runtime_error(message);
    }

    std::vector<uint32_t> spvShader;
    glslang::GlslangToSpv(*program.getIntermediate(stage), spvShader);
    glslang::FinalizeProcess();

    return spvShader;
}

// Support include directive
std::vector<uint32_t> compileToSPV(const std::string& filepath,
                                   const std::vector<Define>& defines) {
    std::string glslCode = File::readFile(filepath);
    EShLanguage stage = translateShaderStage(getShaderStage(filepath));
    std::string included = include(filepath, glslCode);
    addDefines(included, defines);
    return compileToSPV(included, stage);
}

// Don't support include directive
// This is for hardcoded shader in C++
std::vector<uint32_t> compileToSPV(const std::string& glslCode,
                                   vk::ShaderStageFlagBits shaderStage,
                                   const std::vector<Define>& defines) {
    EShLanguage stage = translateShaderStage(shaderStage);
    return compileToSPV(glslCode, stage);
}

std::vector<std::string> getAllIncludedFiles(const std::string& code) {
    std::string includePrefix = "#include \"";
    std::string includeSuffix = "\"";
    std::string::size_type startPos = 0;
    std::vector<std::string> includes;
    while (true) {
        auto includeStartPos = code.find(includePrefix, startPos);
        if (includeStartPos == std::string::npos)
            break;

        auto includeEndPos = code.find(includeSuffix, includeStartPos + includePrefix.length());
        if (includeEndPos == std::string::npos)
            break;

        std::string include =
            code.substr(includeStartPos + includePrefix.length(),
                        includeEndPos - (includeStartPos + includePrefix.length()));
        includes.push_back(include);
        startPos = includeEndPos + includeSuffix.length();
    }
    return includes;
}
}  // namespace Compiler
}  // namespace rv