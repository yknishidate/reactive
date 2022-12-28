#include "Engine.hpp"

struct PushConstants {
    glm::mat4 invView{1};
    glm::mat4 invProj{1};
};

int main() {
    try {
        Log::init();
        Window::init(750, 750);
        Context::init();

        // std::vector<Vertex> vertices{ {{-1, 0, 0}}, {{ 0, -1, 0}}, {{ 1, 0, 0}} };
        // std::vector<Index> indices{ 0, 1, 2 };
        // Mesh mesh{ vertices, indices };

        Swapchain swapchin{};
        GUI gui{swapchin};
        Mesh mesh{"bunny.obj"};
        Object object{mesh};
        TopAccel topAccel{object};
        Camera camera{Window::getWidth(), Window::getHeight()};

        Image outputImage{vk::Format::eB8G8R8A8Unorm};

        Shader rgenShader{SHADER_DIR + "hello_raytracing.rgen"};
        Shader missShader{SHADER_DIR + "hello_raytracing.rmiss"};
        Shader chitShader{SHADER_DIR + "hello_raytracing.rchit"};

        DescriptorSet descSet{};
        descSet.addResources(rgenShader);
        descSet.addResources(missShader);
        descSet.addResources(chitShader);
        descSet.record("topLevelAS", topAccel);
        descSet.record("outputImage", outputImage);
        descSet.allocate();

        RayTracingPipeline pipeline{descSet};
        pipeline.setShaders(rgenShader, missShader, chitShader);
        pipeline.setup(sizeof(PushConstants));

        int testInt = 0;
        int frame = 0;
        while (!Window::shouldClose()) {
            Window::pollEvents();
            gui.startFrame();
            gui.sliderInt("Test slider", testInt, 0, 100);
            camera.processInput();
            object.transform.rotation =
                glm::rotate(glm::mat4(1), 0.01f * frame, glm::vec3(0, 1, 0));
            topAccel.rebuild(object);

            PushConstants pushConstants;
            pushConstants.invProj = camera.getInvProj();
            pushConstants.invView = camera.getInvView();

            swapchin.waitNextFrame();

            CommandBuffer commandBuffer = swapchin.beginCommandBuffer();
            commandBuffer.bindPipeline(pipeline);
            commandBuffer.pushConstants(pipeline, &pushConstants);
            commandBuffer.traceRays(pipeline, Window::getWidth(), Window::getHeight());
            commandBuffer.copyToBackImage(outputImage);
            commandBuffer.beginRenderPass();
            commandBuffer.drawGUI(gui);
            commandBuffer.endRenderPass();
            commandBuffer.submit();

            swapchin.present();
            frame++;
        }
        Context::waitIdle();
        Window::shutdown();
    } catch (const std::exception& e) {
        Log::error(e.what());
    }
}
