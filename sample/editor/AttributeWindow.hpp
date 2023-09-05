#pragma once
#include <imgui.h>

#include "Scene.hpp"

class AttributeWindow {
public:
    void show(Scene& scene, Node* selectedNode) const {
        ImGui::Begin("Attribute");
        if (selectedNode) {
            Transform& transform = selectedNode->transform;
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

            rv::Mesh* mesh = selectedNode->mesh;
            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            if (ImGui::TreeNode(("Mesh: " + mesh->name).c_str())) {
                ImGui::TreePop();
            }

            Material* material = selectedNode->material;
            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            if (ImGui::TreeNode(("Material: " + material->name).c_str())) {
                ImGui::ColorEdit4("Base color", &material->baseColor[0]);
                ImGui::ColorEdit3("Emissive", &material->emissive[0]);
                ImGui::SliderFloat("Metallic", &material->metallic, 0.0f, 1.0f);
                ImGui::SliderFloat("Roughness", &material->roughness, 0.0f, 1.0f);
                ImGui::SliderFloat("IOR", &material->ior, 0.01f, 5.0f);
                ImGui::TreePop();
            }
            // ImGui::EndGroup();
        }
        ImGui::End();
    }
};
