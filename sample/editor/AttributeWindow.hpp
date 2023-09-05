#pragma once
#include <imgui.h>

#include "Scene.hpp"

class AttributeWindow {
public:
    void show(Scene& scene, Node* selectedNode) const {
        ImGui::Begin("Attribute");
        if (selectedNode) {
            Material* material = selectedNode->material;
            ImGui::ColorEdit4("Base color", &material->baseColor[0]);
            ImGui::ColorEdit3("Emissive", &material->emissive[0]);
            ImGui::SliderFloat("Metallic", &material->metallic, 0.0f, 1.0f);
            ImGui::SliderFloat("Roughness", &material->roughness, 0.0f, 1.0f);
            ImGui::SliderFloat("IOR", &material->ior, 0.01f, 5.0f);
        }
        ImGui::End();
    }
};
