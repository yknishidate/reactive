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
        image = context.createImage({
            .usage = ImageUsage::Storage,
            .extent = {Window::getWidth(), Window::getHeight(), 1},
            .format = vk::Format::eB8G8R8A8Unorm,
            .viewInfo = rv::ImageViewCreateInfo{},
        });

        buffer = context.createBuffer({
            .usage = BufferUsage::Uniform,
            .memory = MemoryUsage::Device,
            .size = sizeof(MandelbrotParams),
            .debugName = "buffer",
        });

        context.oneTimeSubmit([&](CommandBufferHandle commandBuffer) {
            commandBuffer->copyBuffer(buffer, &params);
            commandBuffer->transitionLayout(image, vk::ImageLayout::eGeneral);
        });

        SlangCompiler compiler;
        auto codes = compiler.CompileShaders(SHADER_DIR + "shaders.slang", {"computeMain"});

        ShaderHandle compShader = context.createShader({
            .pCode = codes[0]->getBufferPointer(),
            .codeSize = codes[0]->getBufferSize(),
            .stage = vk::ShaderStageFlagBits::eCompute,
        });

        descSet = context.createDescriptorSet({
            .shaders = compShader,
            .buffers = {{"mandelbrotParams", buffer}},
            .images = {{"outputImage", image}},
        });
        descSet->update();

        pipeline = context.createComputePipeline({
            .descSetLayout = descSet->getLayout(),
            .computeShader = compShader,
        });
    }

    void onScroll(float xoffset, float yoffset) override {
        spdlog::info("scroll: {}", yoffset);
        scale *= 1.0f + yoffset * 0.1f;
    }

    void onCursorPos(float xpos, float ypos) override {
        if (Window::isMouseButtonDown(GLFW_MOUSE_BUTTON_1)) {
            spdlog::info("cursor: {}", glm::to_string(Window::getMouseDragLeft()));
            translate +=
                -Window::getMouseDragLeft() / static_cast<float>(Window::getHeight()) / scale;
        }
    }

    void onRender(const CommandBufferHandle& commandBuffer) override {
        float aspect = Window::getWidth() / static_cast<float>(Window::getHeight());
        params.lowerLeft = glm::vec2(-1 * aspect, -1) / scale + translate;
        params.upperRight = glm::vec2(1 * aspect, 1) / scale + translate;
        params.maxIterations = static_cast<int>(scale * 10);

        commandBuffer->copyBuffer(buffer, &params);
        commandBuffer->bindDescriptorSet(pipeline, descSet);
        commandBuffer->bindPipeline(pipeline);
        commandBuffer->dispatch(Window::getWidth(), Window::getHeight(), 1);
        commandBuffer->copyImage(image, getCurrentColorImage(), vk::ImageLayout::eGeneral,
                                 vk::ImageLayout::ePresentSrcKHR);
    }

    float scale = 1.0f;
    glm::vec2 translate = {0.0f, 0.0f};
    MandelbrotParams params;
    ImageHandle image;
    BufferHandle buffer;
    DescriptorSetHandle descSet;
    ComputePipelineHandle pipeline;
};

int main() {
    try {
        HelloApp app{};
        app.run();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}
