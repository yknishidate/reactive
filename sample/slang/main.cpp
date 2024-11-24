#include <reactive/reactive.hpp>
#include <slang/slang.h>
#include <slang/slang-com-ptr.h>

using namespace rv;
using Slang::ComPtr;


inline void diagnoseIfNeeded(slang::IBlob* diagnosticsBlob) {
    if (diagnosticsBlob != nullptr) {
        spdlog::error((const char*)diagnosticsBlob->getBufferPointer());
    }
}

#define ASSERT_ON_SLANG_FAIL(x)               \
    {                                         \
        SlangResult _res = (x);               \
        RV_ASSERT(SLANG_SUCCEEDED(_res), ""); \
    }

std::string vertCode = R"(
#version 450
layout(location = 0) out vec4 outColor;
vec3 positions[] = vec3[](vec3(-1, -1, 0), vec3(0, 1, 0), vec3(1, -1, 0));
vec3 colors[] = vec3[](vec3(0), vec3(1, 0, 0), vec3(0, 1, 0));
void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 1);
    outColor = vec4(colors[gl_VertexIndex], 1);
})";

std::string fragCode = R"(
#version 450
layout(location = 0) in vec4 inColor;
layout(location = 0) out vec4 outColor;
void main() {
    outColor = inColor;
})";

class HelloApp : public App {
public:
    HelloApp()
        : App({
              .width = 1280,
              .height = 720,
              .title = "HelloGraphics",
              .vsync = false,
              .layers = {Layer::Validation, Layer::FPSMonitor},
          }) {}

    void onStart() override {

        // First we need to create slang global session with work with the Slang API.
        Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;
        ASSERT_ON_SLANG_FAIL(slang::createGlobalSession(slangGlobalSession.writeRef()));

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
            auto path = SHADER_DIR + "shaders.slang";
            slangModule = session->loadModule(path.c_str(), diagnosticBlob.writeRef());
            diagnoseIfNeeded(diagnosticBlob);
            if (!slangModule) {
                return;
            }
        }

        ComPtr<slang::IEntryPoint> vertexEntryPoint;
        ASSERT_ON_SLANG_FAIL(slangModule->findEntryPointByName("vertexMain", vertexEntryPoint.writeRef()));

        ComPtr<slang::IEntryPoint> fragmentEntryPoint;
        ASSERT_ON_SLANG_FAIL(slangModule->findEntryPointByName("fragmentMain", fragmentEntryPoint.writeRef()));

            
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

        ComPtr<slang::IBlob> vertexSpirvCode;
        ComPtr<slang::IBlob> fragmentSpirvCode;
        {
            result = linkedProgram->getEntryPointCode(
                vertexEntryPointIndex, 0, vertexSpirvCode.writeRef(), diagnosticBlob.writeRef());
            diagnoseIfNeeded(diagnosticBlob);
            ASSERT_ON_SLANG_FAIL(result);

            result = linkedProgram->getEntryPointCode(
                fragmentEntryPointIndex, 0, fragmentSpirvCode.writeRef(), diagnosticBlob.writeRef());
            diagnoseIfNeeded(diagnosticBlob);
            ASSERT_ON_SLANG_FAIL(result);

            // if (isTestMode()) {
            //     printEntrypointHashes(1, 1, composedProgram);
            // }
        }

        
        std::vector<ShaderHandle> shaders(2);
        shaders[0] = context.createShader({
            .code = static_cast<const uint32_t*>(vertexSpirvCode->getBufferPointer()),
            .stage = vk::ShaderStageFlagBits::eVertex,
        });

        shaders[1] = context.createShader({
            .code = Compiler::compileToSPV(fragCode, vk::ShaderStageFlagBits::eFragment),
            .stage = vk::ShaderStageFlagBits::eFragment,
        });

        descSet = context.createDescriptorSet({
            .shaders = shaders,
        });

        pipeline = context.createGraphicsPipeline({
            .descSetLayout = descSet->getLayout(),
            .vertexShader = shaders[0],
            .fragmentShader = shaders[1],
        });

        gpuTimer = context.createGPUTimer({});
    }

    void onRender(const CommandBufferHandle& commandBuffer) override {
        if (frame > 0) {
            for (int i = 0; i < TIME_BUFFER_SIZE - 1; i++) {
                times[i] = times[i + 1];
            }
            float time = gpuTimer->elapsedInMilli();
            times[TIME_BUFFER_SIZE - 1] = time;
            ImGui::Text("GPU timer: %.3f ms", time);
            ImGui::PlotLines("Times", times, TIME_BUFFER_SIZE, 0, nullptr, FLT_MAX, FLT_MAX,
                             {300, 150});
        }

        commandBuffer->clearColorImage(getCurrentColorImage(), {0.0f, 0.0f, 0.5f, 1.0f});
        commandBuffer->setViewport(Window::getWidth(), Window::getHeight());
        commandBuffer->setScissor(Window::getWidth(), Window::getHeight());
        commandBuffer->bindDescriptorSet(pipeline, descSet);
        commandBuffer->bindPipeline(pipeline);
        commandBuffer->beginTimestamp(gpuTimer);
        commandBuffer->beginRendering(getCurrentColorImage(), nullptr, {0, 0},
                                      {Window::getWidth(), Window::getHeight()});
        commandBuffer->draw(3, 1, 0, 0);
        commandBuffer->endRendering();
        commandBuffer->endTimestamp(gpuTimer);
        frame++;
    }

    static constexpr int TIME_BUFFER_SIZE = 300;
    float times[TIME_BUFFER_SIZE] = {0};
    DescriptorSetHandle descSet;
    GraphicsPipelineHandle pipeline;
    GPUTimerHandle gpuTimer;
    int frame = 0;
};

int main() {
    try {
        HelloApp app{};
        app.run();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}
