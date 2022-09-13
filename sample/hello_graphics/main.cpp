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
            [&]()
            {
                pipeline.Begin();
                vk::DeviceSize offsets{ 0 };
                Graphics::GetCurrentCommandBuffer().bindVertexBuffers(0, mesh->GetVertexBuffer(), offsets);
                Graphics::GetCurrentCommandBuffer().bindIndexBuffer(mesh->GetIndexBuffer(), 0, vk::IndexType::eUint32);
                Graphics::GetCurrentCommandBuffer().drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
                pipeline.End();
            });
    }
    Engine::Shutdown();
}
