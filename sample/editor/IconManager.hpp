#pragma once
#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include "reactive/Graphics/Context.hpp"
#include "reactive/Graphics/Image.hpp"

class IconManager {
public:
    void show(const std::string& name,
              bool showName,
              float thumbnailSize,
              ImVec4 bgColor,
              ImVec4 bgHoverColor,
              const std::function<void()>& callback) {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 buttonMin = ImGui::GetCursorScreenPos();
        ImVec2 buttonMax = ImVec2(buttonMin.x + thumbnailSize, buttonMin.y + thumbnailSize);
        if (mousePos.x >= buttonMin.x &&  // break
            mousePos.y >= buttonMin.y &&  // break
            mousePos.x <= buttonMax.x &&  // break
            mousePos.y <= buttonMax.y) {
            bgColor = bgHoverColor;
        }

        if (ImGui::ImageButton(icons[name].descSet,             // texture
                               {thumbnailSize, thumbnailSize},  // size
                               {0, 0}, {1, 1}, 0, bgColor)) {
            callback();
        }
        if (showName) {
            ImGui::TextWrapped(name.c_str());
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
