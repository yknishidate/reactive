#include <reactive/reactive.hpp>

using namespace rv;

struct MandelbrotParams {
    glm::vec2 lowerLeft = {-1, -1};
    glm::vec2 upperRight = {1, 1};
    int maxIterations = 10;
};

class HelloApp : public App {
public:
    HelloApp()
        : App({
              .width = 1280,
              .height = 720,
              .title = "HelloCompute",
              .layers = {Layer::Validation},
          }) {}

    void onStart() override {
        m_image = m_context.createImage({
            .usage = ImageUsage::Storage,
            .extent = {Window::getWidth(), Window::getHeight(), 1},
            .format = vk::Format::eB8G8R8A8Unorm,
            .viewInfo = rv::ImageViewCreateInfo{},
        });

        m_buffer = m_context.createBuffer({
            .usage = BufferUsage::Uniform,
            .memory = MemoryUsage::Device,
            .size = sizeof(MandelbrotParams),
            .debugName = "m_buffer",
        });

        m_context.oneTimeSubmit([&](CommandBufferHandle commandBuffer) {
            commandBuffer->copyBuffer(m_buffer, &m_params);
            commandBuffer->transitionLayout(m_image, vk::ImageLayout::eGeneral);
        });

        SlangCompiler compiler;
        auto codes = compiler.compileShaders(SHADER_PATH, {"computeMain"});

        ShaderHandle compShader = m_context.createShader({
            .pCode = codes[0]->getBufferPointer(),
            .codeSize = codes[0]->getBufferSize(),
            .stage = vk::ShaderStageFlagBits::eCompute,
        });

        m_descSet = m_context.createDescriptorSet({
            .shaders = compShader,
            .buffers = {{"mandelbrotParams", m_buffer}},
            .images = {{"outputImage", m_image}},
        });
        m_descSet->update();

        m_pipeline = m_context.createComputePipeline({
            .descSetLayout = m_descSet->getLayout(),
            .computeShader = compShader,
        });
    }

    void onScroll(float xoffset, float yoffset) override {
        spdlog::info("scroll: {}", yoffset);
        m_scale *= 1.0f + yoffset * 0.1f;
    }

    void onCursorPos(float xpos, float ypos) override {
        if (Window::isMouseButtonDown(GLFW_MOUSE_BUTTON_1)) {
            spdlog::info("cursor: {}", glm::to_string(Window::getMouseDragLeft()));
            m_translate +=
                -Window::getMouseDragLeft() / static_cast<float>(Window::getHeight()) / m_scale;
        }
    }

    void onRender(const CommandBufferHandle& commandBuffer) override {
        float aspect = Window::getWidth() / static_cast<float>(Window::getHeight());
        m_params.lowerLeft = glm::vec2(-1 * aspect, -1) / m_scale + m_translate;
        m_params.upperRight = glm::vec2(1 * aspect, 1) / m_scale + m_translate;
        m_params.maxIterations = static_cast<int>(m_scale * 10);

        commandBuffer->copyBuffer(m_buffer, &m_params);
        commandBuffer->bindDescriptorSet(m_pipeline, m_descSet);
        commandBuffer->bindPipeline(m_pipeline);
        commandBuffer->dispatch(Window::getWidth(), Window::getHeight(), 1);
        commandBuffer->copyImage(m_image, getCurrentColorImage(), vk::ImageLayout::eGeneral,
                                 vk::ImageLayout::ePresentSrcKHR);
    }

    float m_scale = 1.0f;
    glm::vec2 m_translate = {0.0f, 0.0f};
    MandelbrotParams m_params;
    ImageHandle m_image;
    BufferHandle m_buffer;
    DescriptorSetHandle m_descSet;
    ComputePipelineHandle m_pipeline;
};

int main() {
    try {
        HelloApp app{};
        app.run();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}
