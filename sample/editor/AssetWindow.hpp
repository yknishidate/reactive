#pragma once
#include <imgui.h>

#include "IconManager.hpp"
#include "Scene.hpp"

class AssetWindow {
public:
    void init(const rv::Context& context) {
        iconManager.addIcon(context, "asset_mesh", ASSET_DIR + "icons/asset_mesh.png");
        iconManager.addIcon(context, "asset_material", ASSET_DIR + "icons/asset_material.png");
    }

    void show(const Scene& scene) {
        if (ImGui::Begin("Asset")) {
            float padding = 16.0f;
            float thumbnailSize = 100.0f;
            float cellSize = thumbnailSize + padding;
            float panelWidth = ImGui::GetContentRegionAvail().x;
            int columnCount = std::max(static_cast<int>(panelWidth / cellSize), 1);
            ImGui::Columns(columnCount, 0, false);

            for (auto& mesh : scene.meshes) {
                iconManager.show("asset_mesh", mesh.name, thumbnailSize, ImVec4(0, 0, 0, 1), [] {});
            }

            for (auto& material : scene.materials) {
                iconManager.show("asset_material", material.name, thumbnailSize, ImVec4(0, 0, 0, 1),
                                 [] {});
            }

            ImGui::Columns(1);

            ImGui::End();
        }
    }

    IconManager iconManager;
};
