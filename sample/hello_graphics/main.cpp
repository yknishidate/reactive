#include <boost/hana.hpp>
#include <glm/glm.hpp>
#include <reactive/reactive.hpp>

using namespace rv;
namespace hana = boost::hana;
using namespace std::literals;

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

template <typename T>
void drawParam(const char* name, T* ptr) {
    if constexpr (std::is_same_v<T, std::string>) {
        ImGui::LabelText(name, (*ptr).c_str());
    } else if constexpr (std::is_same_v<T, int>) {
        ImGui::SliderInt(name, ptr, 0, 100);
    } else if constexpr (std::is_same_v<T, float>) {
        ImGui::SliderFloat(name, ptr, 0.0f, 100.0f);
    } else if constexpr (std::is_same_v<T, glm::vec3>) {
        ImGui::SliderFloat3(name, glm::value_ptr(*ptr), -100.0f, 100.0f);
    }
}

template <typename T>
void drawParamsImpl(T& component) {
    if (ImGui::TreeNode(component.structName.c_str())) {
        hana::for_each(hana::keys(component), [&](const auto& key) {
            auto& value = hana::at_key(component, key);
            drawParam(key.c_str(), &value);
        });
        ImGui::TreePop();
    }
}

struct Component {
    virtual ~Component() = default;

    virtual void drawParams() = 0;
};

struct TransformComponent : public Component {
    static inline std::string structName = "TransformComponent";

    BOOST_HANA_DEFINE_STRUCT(  //
        TransformComponent,
        (glm::vec3, translation),
        (glm::vec3, scale),
        (glm::vec3, rotation));

    TransformComponent(glm::vec3 translation = {0.0f, 0.0f, 0.0f},
                       glm::vec3 scale = {1.0f, 1.0f, 1.0f},
                       glm::vec3 rotation = {0.0f, 0.0f, 0.0f})
        : translation(translation), scale(scale), rotation(rotation) {}

    void drawParams() override { drawParamsImpl(*this); }
};

struct PhysicsComponent : public Component {
    static inline std::string structName = "PhysicsComponent";

    BOOST_HANA_DEFINE_STRUCT(  //
        PhysicsComponent,
        (float, mass),
        (glm::vec3, velocity),
        (glm::vec3, acceleration));

    PhysicsComponent(float mass = 0.0f,
                     glm::vec3 velocity = {0.0f, 0.0f, 0.0f},
                     glm::vec3 acceleration = {0.0f, 0.0f, 0.0f})
        : mass(mass), velocity(velocity), acceleration(acceleration) {}

    void drawParams() override { drawParamsImpl(*this); }
};

struct GameObject {
    void drawAttributes() {
        if (ImGui::Begin(name.c_str())) {
            for (auto& component : components) {
                component->drawParams();
            }
            ImGui::End();
        }
    }

    std::string name = "default";
    std::vector<std::unique_ptr<Component>> components;
};

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
        std::vector<ShaderHandle> shaders(2);
        shaders[0] = context.createShader({
            .code = Compiler::compileToSPV(vertCode, vk::ShaderStageFlagBits::eVertex),
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

        object.name = "Player";
        object.components.push_back(std::make_unique<TransformComponent>());
        object.components.push_back(std::make_unique<PhysicsComponent>());
    }

    void onRender(const CommandBufferHandle& commandBuffer) override {
        object.drawAttributes();

        commandBuffer->clearColorImage(getCurrentColorImage(), {0.0f, 0.0f, 0.5f, 1.0f});
        commandBuffer->setViewport(Window::getWidth(), Window::getHeight());
        commandBuffer->setScissor(Window::getWidth(), Window::getHeight());
        commandBuffer->bindDescriptorSet(descSet, pipeline);
        commandBuffer->bindPipeline(pipeline);
        commandBuffer->beginTimestamp(gpuTimer);
        commandBuffer->beginRendering(getCurrentColorImage(), nullptr, {0, 0},
                                      {Window::getWidth(), Window::getHeight()});
        commandBuffer->draw(3, 1, 0, 0);
        commandBuffer->endRendering();
        commandBuffer->endTimestamp(gpuTimer);
    }

    DescriptorSetHandle descSet;
    GraphicsPipelineHandle pipeline;
    GPUTimerHandle gpuTimer;

    GameObject object;
};

int main() {
    try {
        HelloApp app{};
        app.run();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}
