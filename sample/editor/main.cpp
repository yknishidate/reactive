
#include <reactive/App.hpp>

#include "AttributeWindow.hpp"
#include "Scene.hpp"
#include "SceneWindow.hpp"
#include "ViewportWindow.hpp"

using namespace rv;

class Editor : public App {
public:
    Editor()
        : App({
              .width = 2560,
              .height = 1440,
              .title = "Reactive Editor",
              .layers = {Layer::Validation},
          }) {}

    void onStart() override {
        // Add mesh
        scene.meshes.push_back(Mesh::createCubeMesh(context, {}));

        // Add material
        Material material;
        material.baseColor = glm::vec4{1, 0, 0, 1};
        material.name = "Standard 0";
        scene.materials.push_back(material);

        material.baseColor = glm::vec4{1, 1, 0, 1};
        material.name = "Standard 1";
        scene.materials.push_back(material);

        // Add node
        Node node;
        node.name = "Cube 0";
        node.mesh = &scene.meshes.back();
        node.material = &scene.materials[0];
        scene.nodes.push_back(node);

        node.name = "Cube 1";
        node.material = &scene.materials[1];
        scene.nodes.push_back(node);

        camera = OrbitalCamera{this, 1920, 1080};
        camera.fovY = glm::radians(30.0f);
        camera.distance = 10.0f;

        viewportWindow.createPipeline(context);
        viewportWindow.createImages(context, 1920, 1080);
        viewportWindow.createIcons(context);

        icons.push_back(Image::loadFromFile(context, ASSET_DIR + "icons/asset_mesh.png"));
        icons.push_back(Image::loadFromFile(context, ASSET_DIR + "icons/asset_material.png"));
        iconDescSets.emplace_back(ImGui_ImplVulkan_AddTexture(  // break
            icons[IconMesh]->getSampler(), icons[IconMesh]->getView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        iconDescSets.emplace_back(ImGui_ImplVulkan_AddTexture(  // break
            icons[IconMaterial]->getSampler(), icons[IconMaterial]->getView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
    }

    void onUpdate() override {
        camera.processDragDelta(viewportWindow.dragDelta);
        camera.processMouseScroll(viewportWindow.mouseScroll);
        frame++;
    }

    void showAssetIcon(int iconType, float thumbnailSize, const std::string& name) const {
        ImGui::ImageButton(iconDescSets[iconType],          // user_texture_id
                           {thumbnailSize, thumbnailSize},  // size
                           {0, 0},                          // uv0
                           {1, 1},                          // uv1
                           0,                               // frame_padding
                           ImVec4(0, 0, 0, 1)               // bb_col
        );
        ImGui::TextWrapped(name.c_str());
        ImGui::NextColumn();
    }

    void onRender(const CommandBuffer& commandBuffer) override {
        if (viewportWindow.needsRecreate()) {
            context.getDevice().waitIdle();
            viewportWindow.createImages(context, viewportWindow.width, viewportWindow.height);
            camera.aspect = viewportWindow.width / viewportWindow.height;
        }
        static bool dockspaceOpen = true;
        commandBuffer.clearColorImage(getCurrentColorImage(), {0.0f, 0.0f, 0.0f, 1.0f});

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
        windowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        windowFlags |= ImGuiWindowFlags_MenuBar;

        if (dockspaceOpen) {
            ImGui::Begin("DockSpace", &dockspaceOpen, windowFlags);
            ImGui::PopStyleVar(3);

            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */
                    }
                    if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do stuff */
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Edit")) {
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Create")) {
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

            sceneWindow.show(scene, &selectedNode);
            attributeWindow.show(scene, selectedNode);

            if (ImGui::Begin("Asset")) {
                float padding = 16.0f;
                float thumbnailSize = 100.0f;
                float cellSize = thumbnailSize + padding;
                float panelWidth = ImGui::GetContentRegionAvail().x;
                int columnCount = std::max(static_cast<int>(panelWidth / cellSize), 1);
                ImGui::Columns(columnCount, 0, false);

                for (auto& mesh : scene.meshes) {
                    showAssetIcon(IconMesh, thumbnailSize, mesh.name);
                }

                for (auto& material : scene.materials) {
                    showAssetIcon(IconMaterial, thumbnailSize, material.name);
                }

                ImGui::Columns(1);

                ImGui::End();
            }

            viewportWindow.show(commandBuffer, scene, selectedNode, camera, frame);

            ImGui::End();
        }
    }

    // Scene
    OrbitalCamera camera;
    Scene scene;
    int frame = 0;

    // ImGui
    Node* selectedNode = nullptr;

    // Editor
    SceneWindow sceneWindow;
    ViewportWindow viewportWindow;
    AttributeWindow attributeWindow;

    enum IconType {
        IconMesh = 0,
        IconMaterial = 1,
    };

    std::vector<ImageHandle> icons;
    std::vector<vk::DescriptorSet> iconDescSets;
};

int main() {
    try {
        Editor app{};
        app.run();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}
