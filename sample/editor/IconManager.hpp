#pragma once
#include <imgui.h>
#include <imgui_impl_vulkan.h>

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

    void show(const std::string& iconName,
              const std::string& itemName,
              float thumbnailSize,
              ImVec4 bgColor,
              const std::function<void()>& callback) {
        if (ImGui::ImageButton(icons[iconName].descSet,         // texture
                               {thumbnailSize, thumbnailSize},  // size
                               {0, 0}, {1, 1}, 0, bgColor)) {
            callback();
        }
        if (!itemName.empty()) {
            ImGui::TextWrapped(itemName.c_str());
        }
        ImGui::NextColumn();
    }

    void addIcon(const rv::Context& context, const std::string& name, const std::string& filepath) {
        icons[name].image = rv::Image::loadFromFile(context, filepath);
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
