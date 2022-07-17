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
    Engine::Init(1500, 1500);

    std::vector<Vertex> vertices{
        {{ -1, -1, 0}, {}, {0, 0}},
        {{  1, -1, 0}, {}, {1, 0}},
        {{  1,  1, 0}, {}, {1, 1}},
        {{ -1,  1, 0}, {}, {0, 1}},
    };
    std::vector<Index> indices{ 0, 1, 2, 0, 2, 3 };
    auto mesh = std::make_shared<Mesh>(vertices, indices);

    //auto imagePaths = GetAllFilePaths(ASSET_DIR + "chess_lf");
    auto imagePaths = GetAllFilePaths(ASSET_DIR + "lego_lf");
    Image images{ imagePaths };
    Image outputImage{ vk::Format::eB8G8R8A8Unorm };
    images.SetSamplerMode(vk::SamplerAddressMode::eClampToEdge);

    Scene scene{};
    auto& cameraPlane = scene.AddObject(mesh);
    cameraPlane.transform.scale *= 5.0f;

    int scale = 3;
    glm::vec3 defaultScale = { images.GetAspect(), 1, 1 };
    auto& focalPlane = scene.AddObject(mesh);
    focalPlane.transform.position = { 0.0, 0.0, -scale };
    focalPlane.transform.scale = defaultScale * static_cast<float>(scale);

    scene.Setup();
    scene.SetCamera<OrbitalCamera>();

    RayTracingPipeline pipeline{};
    pipeline.LoadShaders(SHADER_DIR + "light_field.rgen",
                         SHADER_DIR + "light_field.rmiss",
                         SHADER_DIR + "light_field.rchit");
    pipeline.GetDescSet().Register("topLevelAS", scene.GetAccel());
    pipeline.GetDescSet().Register("outputImage", outputImage);
    pipeline.GetDescSet().Register("images", images);
    pipeline.GetDescSet().Setup();
    pipeline.Setup(sizeof(PushConstants));

    PushConstants pushConstants;
    pushConstants.vertices = mesh->GetVertexBufferAddress();
    pushConstants.indices = mesh->GetIndexBufferAddress();

    int fov = 20;
    while (Engine::Update()) {
        scene.Update(0.1f);

        GUI::SliderInt("FOV", fov, 20, 80);
        bool changed = GUI::SliderInt("Far plane", scale, 1, 25);
        scene.GetCamera().SetFov(static_cast<float>(fov));
        focalPlane.transform.position = { 0.0, 0.0, -scale };
        focalPlane.transform.scale = defaultScale * static_cast<float>(scale);
        if (changed) scene.Rebuild();

        pushConstants.invProj = scene.GetCamera().GetInvProj();
        pushConstants.invView = scene.GetCamera().GetInvView();
        Engine::Render([&]() {
            pipeline.Run(&pushConstants);
            outputImage.CopyToBackImage(); });
    }
    Engine::Shutdown();
}
