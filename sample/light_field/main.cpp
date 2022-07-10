#include "Engine.hpp"

struct PushConstants
{
    //int rows = 17;
    //int cols = 17;
    //glm::vec2 st = { 0.5, 0.5 };
    glm::mat4 invView;
    glm::mat4 invProj;
    vk::DeviceAddress vertices;
    vk::DeviceAddress indices;

    void HandleInput()
    {
        //if (!Window::MousePressed()) return;
        //auto motion = Window::GetMouseMotion() * 0.005f;
        //st = clamp(st + glm::vec2(-motion.x, motion.y), 0.0f, 1.0f);
    }
};

namespace fs = std::filesystem;
std::vector<std::string> GetAllFilePaths(const fs::path& directory)
{
    std::vector<std::string> imagePaths;
    for (const auto& file : fs::directory_iterator{ directory }) {
        imagePaths.push_back(file.path().string());
    }
    return imagePaths;
}

int main()
{
    Engine::Init(720, 720);

    std::vector<Vertex> vertices{
        {{ -1, -1, 0}, {}, {0, 0}},
        {{  1, -1, 0}, {}, {1, 0}},
        {{  1,  1, 0}, {}, {1, 1}},
        {{ -1,  1, 0}, {}, {0, 1}},
    };
    std::vector<Index> indices{ 0, 1, 2, 0, 2, 3 };
    auto mesh = std::make_shared<Mesh>(vertices, indices);

    Scene scene{};
    auto& stPlane = scene.AddObject(mesh);
    auto& uvPlane = scene.AddObject(mesh);
    uvPlane.transform.scale = { 2.8, 1.6, 2.0 };
    uvPlane.transform.position = { 0.0, 0.0, -3.0 };
    scene.Setup();

    //auto imagePaths = GetAllFilePaths(ASSET_DIR + "chess_lf");
    //Image images{ imagePaths };
    Image outputImage{ vk::Format::eB8G8R8A8Unorm };

    RayTracingPipeline pipeline{};
    pipeline.LoadShaders(SHADER_DIR + "light_field.rgen",
                         SHADER_DIR + "light_field.rmiss",
                         SHADER_DIR + "light_field.rchit");
    pipeline.GetDescSet().Register("topLevelAS", scene.GetAccel());
    pipeline.GetDescSet().Register("outputImage", outputImage);
    //pipeline.GetDescSet().Register("images", images);
    pipeline.GetDescSet().Setup();
    pipeline.Setup(sizeof(PushConstants));

    PushConstants pushConstants;
    pushConstants.vertices = mesh->GetVertexBufferAddress();
    pushConstants.indices = mesh->GetIndexBufferAddress();

    while (Engine::Update()) {
        scene.Update(0.1f);

        pushConstants.invProj = scene.GetCamera().GetInvProj();
        pushConstants.invView = scene.GetCamera().GetInvView();
        //pushConstants.HandleInput();
        Engine::Render([&]() {
            pipeline.Run(&pushConstants);
            outputImage.CopyToBackImage(); });
    }
    Engine::Shutdown();
}
