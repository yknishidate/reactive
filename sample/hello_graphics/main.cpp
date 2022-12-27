#include "Engine.hpp"

struct PushConstants
{
    glm::mat4 model{ 1 };
    glm::mat4 view{ 1 };
    glm::mat4 proj{ 1 };
};

int main()
{
    try {
        Log::init();
        Window::init(750, 750);
        Context::init();
        GUI::init();

        std::vector<Vertex> vertices{ {{-1, 0, 0}}, {{ 0, -1, 0}}, {{ 1, 0, 0}} };
        std::vector<Index> indices{ 0, 1, 2 };
        auto mesh = std::make_shared<Mesh>(vertices, indices);
        Camera camera{ Window::getWidth(), Window::getHeight() };

        GraphicsPipeline pipeline{ };
        pipeline.loadShaders(SHADER_DIR + "hello_graphics.vert",
                             SHADER_DIR + "hello_graphics.frag");
        pipeline.setup(sizeof(PushConstants));

        int testInt = 0;
        int frame = 0;
        while (!Window::shouldClose()) {
            Window::pollEvents();
            camera.processInput();

            PushConstants pushConstants;
            pushConstants.model = glm::rotate(glm::mat4(1.0f), 0.01f * frame, glm::vec3(0.0f, 1.0f, 0.0f));
            pushConstants.proj = camera.getProj();
            pushConstants.view = camera.getView();

            Context::waitNextFrame();
            vk::CommandBuffer commandBuffer = Context::beginCommandBuffer();
            pipeline.begin(commandBuffer, &pushConstants);

            Image::setImageLayout(commandBuffer, Context::getSwapchain().getBackImage(),
                                  vk::ImageLayout::eUndefined,
                                  vk::ImageLayout::eTransferDstOptimal);
            commandBuffer.clearColorImage(Context::getSwapchain().getBackImage(),
                                          vk::ImageLayout::eTransferDstOptimal,
                                          vk::ClearColorValue{ std::array{0.0f, 0.0f, 0.3f, 1.0f} },
                                          vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
            Context::beginRenderPass();
            vk::DeviceSize offsets{ 0 };
            commandBuffer.bindVertexBuffers(0, mesh->getVertexBuffer(), offsets);
            commandBuffer.bindIndexBuffer(mesh->getIndexBuffer(), 0, vk::IndexType::eUint32);
            commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

            GUI::startFrame();
            GUI::sliderInt("Test slider", testInt, 0, 100);
            GUI::render(commandBuffer);

            Context::endRenderPass();

            Context::submit();
            Context::present();
            frame++;
        }
        Context::getDevice().waitIdle();
        GUI::shutdown();
        Window::shutdown();
    } catch (const std::exception& e) {
        Log::error(e.what());
    }
}
