
#include <future>
#include <reactive/App.hpp>

#include "AssetWindow.hpp"
#include "AttributeWindow.hpp"
#include "RenderWindow.hpp"
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
              .extensions = {Extension::RayTracing},
          }) {}

    void onStart() override {
        // Add mesh
        scene.meshes.push_back(Mesh::createCubeMesh(context, {}));
        scene.meshes.push_back(Mesh::createPlaneMesh(context, {.width = 10.0f, .height = 10.0f}));

        // Add material
        Material material;
        material.baseColor = glm::vec4{0.9, 0.3, 0.2, 1};
        material.name = "Standard 0";
        scene.materials.push_back(material);

        material.baseColor = glm::vec4{0.8, 0.7, 0.1, 1};
        material.name = "Standard 1";
        scene.materials.push_back(material);

        material.baseColor = glm::vec4{0.9, 0.9, 0.9, 1};
        material.name = "Standard 2";
        scene.materials.push_back(material);

        material.baseColor = glm::vec4{0, 0, 0, 1};
        material.emissive = glm::vec3{0.95, 0.95, 0.95};
        material.name = "Dome light 0";
        scene.materials.push_back(material);

        // Add node
        Node node;
        node.name = "Cube 0";
        node.mesh = &scene.meshes[0];
        node.material = &scene.materials[0];
        node.transform.translation = glm::vec3{-1.5, 0, 0};
        scene.nodes.push_back(node);

        node.name = "Cube 1";
        node.material = &scene.materials[1];
        node.transform.translation = glm::vec3{1.5, 0, 0};
        scene.nodes.push_back(node);

        node.name = "Plane 0";
        node.mesh = &scene.meshes[1];
        node.material = &scene.materials[2];
        node.transform.translation = glm::vec3{0, -1, 0};
        scene.nodes.push_back(node);

        node.name = "Dome light";
        node.type = Node::DomeLightNode;
        node.mesh = nullptr;
        node.material = &scene.materials[3];
        node.transform.translation = glm::vec3{0, 0, 0};
        scene.nodes.push_back(node);

        camera = OrbitalCamera{this, 1920, 1080};
        camera.fovY = glm::radians(30.0f);
        camera.distance = 10.0f;

        iconManager.init(context);
        assetWindow.init(context, scene, iconManager);
        viewportWindow.init(context, iconManager, 1920, 1080);
        renderWindow.init(context, scene, 1280, 720);
        attributeWindow.init(context, scene, iconManager);
    }

    void onUpdate() override {
        camera.processDragDelta(viewportWindow.dragDelta);
        camera.processMouseScroll(viewportWindow.mouseScroll);
        frame++;
    }

    void onRender(const CommandBuffer& commandBuffer) override {
        if (viewportWindow.needsRecreate()) {
            context.getDevice().waitIdle();
            viewportWindow.createImages(viewportWindow.width, viewportWindow.height);
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
            int message = Message::None;
            message |= attributeWindow.show(selectedNode);
            message |= viewportWindow.show(scene, selectedNode, camera, frame);
            assetWindow.show();
            renderWindow.show(camera, frame);

            viewportWindow.drawContent(commandBuffer, scene, camera, frame);

            renderWindow.render(camera, frame, message);

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
    AssetWindow assetWindow;
    RenderWindow renderWindow;
    IconManager iconManager;
};

int main() {
    try {
        Editor app{};
        app.run();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}
