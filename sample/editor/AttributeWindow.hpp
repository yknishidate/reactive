#pragma once
#include <imgui.h>

#include "Scene.hpp"

class AttributeWindow {
public:
    void showTransform(Node* node) const {
        Transform& transform = node->transform;
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("Transform")) {
            // Translation
            ImGui::DragFloat3("Translation", glm::value_ptr(transform.translation), 0.01);

            // Rotation
            glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(transform.rotation));
            ImGui::DragFloat3("Rotation", glm::value_ptr(eulerAngles), 1.0);
            transform.rotation = glm::quat(glm::radians(eulerAngles));

            // Scale
            ImGui::DragFloat3("Scale", glm::value_ptr(transform.scale), 0.01);

            ImGui::TreePop();
        }
    }

    void showMaterial(const Node* node) const {
        Material* material = node->material;
        if (!material) {
            return;
        }
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode(("Material: " + material->name).c_str())) {
            ImGui::ColorEdit4("Base color", &material->baseColor[0]);
            ImGui::ColorEdit3("Emissive", &material->emissive[0]);
            ImGui::SliderFloat("Metallic", &material->metallic, 0.0f, 1.0f);
            ImGui::SliderFloat("Roughness", &material->roughness, 0.0f, 1.0f);
            ImGui::SliderFloat("IOR", &material->ior, 0.01f, 5.0f);
            ImGui::TreePop();
        }
    }

    void show(Scene& scene, Node* node) const {
        ImGui::Begin("Attribute");
        if (node) {
            showTransform(node);

            rv::Mesh* mesh = node->mesh;
            if (mesh) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                if (ImGui::TreeNode(("Mesh: " + mesh->name).c_str())) {
                    ImGui::TreePop();
                }
            }

            showMaterial(node);
        }
        ImGui::End();
    }
};
