#include "Engine.hpp"

int main()
{
    try {
        Log::init();
        Window::init(750, 750);
        Context::init();
        //GUI::init();

        std::vector<Vertex> vertices{ {{-1, 0, 0}}, {{ 0, -1, 0}}, {{ 1, 0, 0}} };
        std::vector<Index> indices{ 0, 1, 2 };
        auto mesh = std::make_shared<Mesh>(vertices, indices);

        GraphicsPipeline pipeline{ };
        pipeline.loadShaders(SHADER_DIR + "hello_graphics.vert",
                             SHADER_DIR + "hello_graphics.frag");
        pipeline.setup();

        while (!Window::shouldClose()) {
            Window::pollEvents();

            Context::waitNextFrame();
            vk::CommandBuffer commandBuffer = Context::beginCommandBuffer();
            pipeline.begin(commandBuffer);
            Context::getSwapchain().beginRenderPass();
            vk::DeviceSize offsets{ 0 };
            commandBuffer.bindVertexBuffers(0, mesh->getVertexBuffer(), offsets);
            commandBuffer.bindIndexBuffer(mesh->getIndexBuffer(), 0, vk::IndexType::eUint32);
            commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
            Context::getSwapchain().endRenderPass();
            //pipeline.end(commandBuffer);
            //GUI::render(commandBuffer);
            Context::submit();
            Context::present();
        }
        Context::getDevice().waitIdle();
        //GUI::shutdown();
        Window::shutdown();
    } catch (const std::exception& e) {
        Log::error(e.what());
    }
}
