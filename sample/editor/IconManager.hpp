#pragma once
#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include <utility>

#include "reactive/Graphics/Context.hpp"
#include "reactive/Graphics/Image.hpp"

class IconManager {
public:
    bool isHover(float thumbnailSize) const {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 buttonMin = ImGui::GetCursorScreenPos();
        ImVec2 buttonMax = ImVec2(buttonMin.x + thumbnailSize, buttonMin.y + thumbnailSize);
        return mousePos.x >= buttonMin.x && mousePos.y >= buttonMin.y &&
               mousePos.x <= buttonMax.x && mousePos.y <= buttonMax.y;
    }

    ImVec2 getImageSize(const std::string& iconName) {
        vk::Extent3D extent = icons[iconName].image->getExtent();
        return {static_cast<float>(extent.width), static_cast<float>(extent.height)};
    }

    std::pair<ImVec2, ImVec2> computeCenterCroppedUVs(ImVec2 imageSize) const {
        float aspectRatioImage = imageSize.x / imageSize.y;
        float aspectRatioButton = 1.0f;

        ImVec2 uv0 = ImVec2(0, 0);
        ImVec2 uv1 = ImVec2(1, 1);

        if (aspectRatioImage > aspectRatioButton) {
            float offset = (1.0f - aspectRatioButton / aspectRatioImage) * 0.5f;
            uv0.x += offset;
            uv1.x -= offset;
        } else {
            float offset = (1.0f - aspectRatioImage / aspectRatioButton) * 0.5f;
            uv0.y += offset;
            uv1.y -= offset;
        }
        return {uv0, uv1};
    }

    void showIcon(
        const std::string& iconName,
        const std::string& itemName,
        float thumbnailSize,
        ImVec4 bgColor,
        const std::function<void()>& callback = [] {}) {
        auto [uv0, uv1] = computeCenterCroppedUVs(getImageSize(iconName));
        ImTextureID textureId = icons[iconName].descSet;
        ImGui::PushID(itemName.c_str());
        if (ImGui::ImageButton(textureId,                       // texture
                               {thumbnailSize, thumbnailSize},  // size
                               uv0, uv1, 0, bgColor)) {
            callback();
        }
        ImGui::PopID();

        if (!itemName.empty()) {
            ImGui::TextWrapped(itemName.c_str());
        }
        ImGui::NextColumn();
    }

    void showDraggableIcon(
        const std::string& iconName,
        const std::string& itemName,
        float thumbnailSize,
        ImVec4 bgColor,
        const std::function<void()>& callback = [] {}) {
        auto [uv0, uv1] = computeCenterCroppedUVs(getImageSize(iconName));
        ImTextureID textureId = icons[iconName].descSet;
        ImGui::PushID(itemName.c_str());
        if (ImGui::ImageButton(textureId,                       // texture
                               {thumbnailSize, thumbnailSize},  // size
                               uv0, uv1, 0, bgColor)) {
            callback();
            spdlog::info("Click: {}", itemName);
        }

        // Draggable
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("DND_IMAGE", &textureId, sizeof(ImTextureID));
            ImGui::Text(itemName.c_str());
            spdlog::info("Drag: {}", itemName);
            ImGui::EndDragDropSource();
        }
        ImGui::PopID();

        if (!itemName.empty()) {
            ImGui::TextWrapped(itemName.c_str());
        }
        ImGui::NextColumn();
    }

    void showDroppableIcon(
        const std::string& iconName,
        const std::string& itemName,
        float thumbnailSize,
        ImVec4 bgColor,
        const std::function<void()>& callback = [] {},
        const std::function<void(ImTextureID)>& dropCallback = [](ImTextureID) {}) {
        auto [uv0, uv1] = computeCenterCroppedUVs(getImageSize(iconName));
        ImTextureID textureId = icons[iconName].descSet;
        ImGui::PushID(itemName.c_str());
        if (ImGui::ImageButton(textureId,                       // texture
                               {thumbnailSize, thumbnailSize},  // size
                               uv0, uv1, 0, bgColor)) {
            callback();
            spdlog::info("Click: {}", itemName);
        }

        // Droppable
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_IMAGE")) {
                IM_ASSERT(payload->DataSize == sizeof(ImTextureID));
                ImTextureID textureIdDropped = *static_cast<ImTextureID*>(payload->Data);
                dropCallback(textureIdDropped);
                spdlog::info("Drop: {}", itemName);
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::PopID();

        if (!itemName.empty()) {
            ImGui::TextWrapped(itemName.c_str());
        }
        ImGui::NextColumn();
    }

    // With loading. For UI icon
    void addIcon(const rv::Context& context, const std::string& name, const std::string& filepath) {
        icons[name].image = rv::Image::loadFromFile(context, filepath);
        icons[name].descSet = ImGui_ImplVulkan_AddTexture(  // break
            icons[name].image->getSampler(), icons[name].image->getView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    // Without loading. For texture
    void addIcon(const std::string& name, rv::ImageHandle texture) {
        // WARN: this image has ownership
        icons[name].image = std::move(texture);
        icons[name].descSet = ImGui_ImplVulkan_AddTexture(  // break
            icons[name].image->getSampler(), icons[name].image->getView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    struct IconData {
        rv::ImageHandle image;
        vk::DescriptorSet descSet;
    };

    std::unordered_map<std::string, IconData> icons;
};
