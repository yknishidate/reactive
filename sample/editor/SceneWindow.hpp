#pragma once
#include <imgui.h>

#include "Scene.hpp"

class SceneWindow {
public:
    void show(Scene& scene, Node** selectedNode) const {
        ImGui::Begin("Scene");

        for (auto& node : scene.nodes) {
            // Set flag
            int flag = ImGuiTreeNodeFlags_OpenOnArrow;
            if (&node == *selectedNode) {
                flag = flag | ImGuiTreeNodeFlags_Selected;
            }

            // Show node
            bool open = ImGui::TreeNodeEx(node.name.c_str(), flag);
            if (ImGui::IsItemClicked()) {
                *selectedNode = &node;
            }
            if (open) {
                ImGui::TreePop();
            }
        }

        ImGui::End();
    }
};
