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

        //std::vector<Vertex> vertices{ {{-1, 0, 0}}, {{ 0, -1, 0}}, {{ 1, 0, 0}} };
        //std::vector<Index> indices{ 0, 1, 2 };
        //Mesh mesh{ vertices, indices };

        Mesh mesh{ "bunny.obj" };
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
            pipeline.bind(commandBuffer);
            pipeline.pushConstants(commandBuffer, &pushConstants);

            Context::clearBackImage(commandBuffer, { 0.0f, 0.0f, 0.3f, 1.0f });

            Context::beginRenderPass();
            mesh.drawIndexed(commandBuffer);
            GUI::startFrame();
            GUI::sliderInt("Test slider", testInt, 0, 100);
            GUI::render(commandBuffer);
            Context::endRenderPass();

            Context::submit();
            Context::present();
            frame++;
        }
        Context::waitIdle();
        GUI::shutdown();
        Window::shutdown();
    } catch (const std::exception& e) {
        Log::error(e.what());
    }
}
