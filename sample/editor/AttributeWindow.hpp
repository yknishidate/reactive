#pragma once
#include <imgui.h>

#include "Scene.hpp"

namespace Message {
enum Type {
    None = 0,
    TransformChanged = 1 << 0,
    MaterialChanged = 1 << 1,
    CameraChanged = 1 << 2,
    TextureAdded = 1 << 3,
};
}

class AttributeWindow {
public:
    void init(const rv::Context& context, Scene& scene, IconManager& iconManager) {
        this->context = &context;
        this->scene = &scene;
        this->iconManager = &iconManager;
        iconManager.addIcon(context, "asset_texture", ASSET_DIR + "icons/asset_texture.png");
    }

    bool showTransform(Node* node) const {
        Transform& transform = node->transform;
        bool changed = false;
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("Transform")) {
            // Translation
            changed |=
                ImGui::DragFloat3("Translation", glm::value_ptr(transform.translation), 0.01);

            // Rotation
            glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(transform.rotation));
            changed |= ImGui::DragFloat3("Rotation", glm::value_ptr(eulerAngles), 1.0);
            transform.rotation = glm::quat(glm::radians(eulerAngles));

            // Scale
            changed |= ImGui::DragFloat3("Scale", glm::value_ptr(transform.scale), 0.01);

            ImGui::TreePop();
        }
        return changed;
    }

    bool showMaterial(const Node* node) const {
        Material* material = node->material;
        if (!material) {
            return false;
        }
        bool changed = false;
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode(("Material: " + material->name).c_str())) {
            changed |= ImGui::ColorEdit4("Base color", &material->baseColor[0]);

            // Base color texture
            std::string name = "asset_texture";
            if (material->baseColorTextureIndex != -1) {
                name = scene->textures[material->baseColorTextureIndex].name;
            }
            iconManager->showDroppableIcon(
                name, "", 100.0f, ImVec4(0, 0, 0, 1), [] {},
                [&](const char* droppedName) {
                    for (int i = 0; i < scene->textures.size(); i++) {
                        Texture& texture = scene->textures[i];
                        if (std::strcmp(texture.name.c_str(), droppedName) == 0) {
                            node->material->baseColorTextureIndex = i;
                            spdlog::info("[UI] Apply base color texture: {}", droppedName);
                            changed = true;
                        }
                    }
                });

            ImGui::Spacing();

            changed |= ImGui::ColorEdit3("Emissive", &material->emissive[0]);
            changed |= ImGui::SliderFloat("Metallic", &material->metallic, 0.0f, 1.0f);
            changed |= ImGui::SliderFloat("Roughness", &material->roughness, 0.0f, 1.0f);
            changed |= ImGui::SliderFloat("IOR", &material->ior, 0.01f, 5.0f);

            ImGui::TreePop();
        }
        return changed;
    }

    int show(Node* node) {
        ImGui::Begin("Attribute");
        int message = Message::None;
        if (node) {
            if (showTransform(node)) {
                spdlog::info("Transform changed");
                message |= Message::TransformChanged;
            }

            rv::Mesh* mesh = node->mesh;
            if (mesh) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                if (ImGui::TreeNode(("Mesh: " + mesh->name).c_str())) {
                    ImGui::TreePop();
                }
            }

            if (showMaterial(node)) {
                message |= Message::MaterialChanged;
            }
        }
        ImGui::End();
        return message;
    }

    const rv::Context* context = nullptr;
    Scene* scene = nullptr;
    IconManager* iconManager = nullptr;
};
