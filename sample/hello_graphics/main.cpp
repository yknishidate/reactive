#include "Engine.hpp"

int main()
{
    Engine::Init(750, 750);

    std::vector<Vertex> vertices{ {{-1, 0, 0}}, {{ 0, -1, 0}}, {{ 1, 0, 0}} };
    std::vector<Index> indices{ 0, 1, 2 };
    auto mesh = std::make_shared<Mesh>(vertices, indices);

    GraphicsPipeline pipeline{ };
    pipeline.LoadShaders(SHADER_DIR + "hello_graphics.vert",
                         SHADER_DIR + "hello_graphics.frag");
    pipeline.Setup();

    while (Engine::Update()) {
        Engine::Render(
            [&](auto commandBuffer)
            {
                pipeline.Begin(commandBuffer);
                vk::DeviceSize offsets{ 0 };
                commandBuffer.bindVertexBuffers(0, mesh->GetVertexBuffer(), offsets);
                commandBuffer.bindIndexBuffer(mesh->GetIndexBuffer(), 0, vk::IndexType::eUint32);
                commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
                pipeline.End(commandBuffer);
            });
    }
    Engine::Shutdown();
}
