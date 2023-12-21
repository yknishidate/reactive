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
            .extent = {width, height, 1},
            .format = vk::Format::eB8G8R8A8Unorm,
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

        ShaderHandle compShader = context.createShader({
            .code = Compiler::compileToSPV(SHADER_DIR + "hello_compute.comp"),
            .stage = vk::ShaderStageFlagBits::eCompute,
        });

        descSet = context.createDescriptorSet({
            .shaders = compShader,
            .buffers = {{"MandelbrotParams", buffer}},
            .images = {{"outputImage", image}},
        });

        pipeline = context.createComputePipeline({
            .computeShader = compShader,
            .descSetLayout = descSet->getLayout(),
        });
    }

    void onScroll(float xoffset, float yoffset) override {
        spdlog::info("scroll: {}", yoffset);
        scale *= 1.0f + yoffset * 0.1f;
    }

    void onCursorPos(float xpos, float ypos) override {
        static glm::vec2 lastCursorPos{0.0f};
        glm::vec2 cursorPos = {xpos, ypos};
        glm::vec2 cursorOffset = cursorPos - lastCursorPos;
        lastCursorPos = cursorPos;
        if (isMouseButtonDown(GLFW_MOUSE_BUTTON_1)) {
            spdlog::info("cursor: {}", glm::to_string(cursorOffset));
            translate += -cursorOffset / static_cast<float>(height) / scale;
        }
    }

    void onRender(const CommandBufferHandle& commandBuffer) override {
        float aspect = width / static_cast<float>(height);
        params.lowerLeft = glm::vec2(-1 * aspect, -1) / scale + translate;
        params.upperRight = glm::vec2(1 * aspect, 1) / scale + translate;
        params.maxIterations = static_cast<int>(scale * 10);

        commandBuffer->copyBuffer(buffer, &params);
        commandBuffer->bindDescriptorSet(descSet, pipeline);
        commandBuffer->bindPipeline(pipeline);
        commandBuffer->dispatch(width, height, 1);
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
