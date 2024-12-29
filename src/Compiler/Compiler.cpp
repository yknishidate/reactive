#include "reactive/Compiler/Compiler.hpp"
#include <filesystem>
#include <fstream>

#define ASSERT_ON_SLANG_FAIL(x)        \
    {                                  \
        SlangResult _res = (x);        \
        assert(SLANG_SUCCEEDED(_res)); \
    }

namespace rv {
namespace File {
auto readFile(const std::filesystem::path& path) -> std::string {
    std::ifstream input_file{path};
    if (!input_file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path.string());
    }
    return {(std::istreambuf_iterator<char>{input_file}), std::istreambuf_iterator<char>{}};
}
}  // namespace File

static void diagnoseIfNeeded(slang::IBlob* diagnosticsBlob) {
    if (diagnosticsBlob != nullptr) {
        spdlog::error((const char*)diagnosticsBlob->getBufferPointer());
    }
}

SlangCompiler::SlangCompiler() {
    ASSERT_ON_SLANG_FAIL(slang::createGlobalSession(slangGlobalSession.writeRef()));
}

std::vector<Slang::ComPtr<slang::IBlob>> SlangCompiler::CompileShaders(const std::filesystem::path& shaderPath,
                                                                       const std::vector<std::string>& entryPointNames) {
    assert(slangGlobalSession);

    spdlog::info("Compiling: {}", shaderPath.string());

    // 1. Slangセッションの準備
    // ----------------------------------------------------
    slang::TargetDesc targetDesc = {};
    targetDesc.format = SLANG_SPIRV;
    targetDesc.profile = slangGlobalSession->findProfile("spirv_1_5");
    targetDesc.flags = 0;

    SlangCapabilityID spvImageQueryId = slangGlobalSession->findCapability("spvImageQuery");
    SlangCapabilityID spvSparseResidencyId =
        slangGlobalSession->findCapability("spvSparseResidency");

    std::vector<slang::CompilerOptionEntry> options;
    options.push_back(
        {slang::CompilerOptionName::EmitSpirvDirectly,
         {slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}});
    options.push_back(
        {slang::CompilerOptionName::Capability,
         {slang::CompilerOptionValueKind::String, spvImageQueryId, 0, nullptr, nullptr}});
    options.push_back(
        {slang::CompilerOptionName::Capability,
         {slang::CompilerOptionValueKind::String, spvSparseResidencyId, 0, nullptr, nullptr}});

    slang::SessionDesc sessionDesc = {};
    sessionDesc.targets = &targetDesc;
    sessionDesc.targetCount = 1;
    sessionDesc.compilerOptionEntries = options.data();
    sessionDesc.compilerOptionEntryCount = (uint32_t)options.size();

    Slang::ComPtr<slang::ISession> session;
    ASSERT_ON_SLANG_FAIL(slangGlobalSession->createSession(sessionDesc, session.writeRef()));

    // 2. モジュールをロード (Slangソースをコンパイル)
    // ----------------------------------------------------
    Slang::ComPtr<slang::IBlob> diagnosticBlob;
    slang::IModule* slangModule = nullptr;
    {
        slangModule = session->loadModule(shaderPath.string().c_str(), diagnosticBlob.writeRef());
        diagnoseIfNeeded(diagnosticBlob);
        if (!slangModule) {
            // 失敗した場合は空の結果を返して終了
            return {};
        }
    }

    // 3. 各エントリーポイントを取り出す
    // ----------------------------------------------------
    //    - まず module (IComponentType) としてのslangModuleを追加
    //    - 各 entryPointName から IEntryPoint を見つけて追加
    std::vector<slang::IComponentType*> componentTypes;
    componentTypes.reserve(entryPointNames.size() + 1);

    // 最初にモジュール本体を入れる
    componentTypes.push_back(slangModule);

    for (const auto& name : entryPointNames) {
        Slang::ComPtr<slang::IEntryPoint> entryPoint;
        SlangResult res = slangModule->findEntryPointByName(name.c_str(), entryPoint.writeRef());
        diagnoseIfNeeded(diagnosticBlob);
        ASSERT_ON_SLANG_FAIL(res);

        // コンポーネントとして追加
        componentTypes.push_back(entryPoint);
    }

    // 4. まとめてコンポジットを作る (リンクする)
    // ----------------------------------------------------
    Slang::ComPtr<slang::IComponentType> linkedProgram;
    {
        SlangResult result = session->createCompositeComponentType(
            componentTypes.data(), componentTypes.size(), linkedProgram.writeRef(),
            diagnosticBlob.writeRef());
        diagnoseIfNeeded(diagnosticBlob);
        ASSERT_ON_SLANG_FAIL(result);
    }

    // 5. 各エントリーポイントのコードを取得
    // ----------------------------------------------------
    //    - entryPointIndices と同じ順番で出力用のバイナリを取り出す
    std::vector<Slang::ComPtr<slang::IBlob>> codes(entryPointNames.size());

    for (size_t i = 0; i < entryPointNames.size(); i++) {
        // インデックスは "モジュール以降のエントリーポイント" の順序
        SlangResult result = linkedProgram->getEntryPointCode(i, 0, codes[i].writeRef(),
                                                              diagnosticBlob.writeRef());
        diagnoseIfNeeded(diagnosticBlob);
        ASSERT_ON_SLANG_FAIL(result);
    }

    spdlog::info("  Done!");
    return codes;
}

}  // namespace rv
