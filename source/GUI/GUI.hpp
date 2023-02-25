#pragma once
#include <imgui.h>
#include <glm/glm.hpp>

class Swapchain;

class GUI {
public:
    GUI(Swapchain& swapchain);
    ~GUI();

    void startFrame();
    void render(vk::CommandBuffer commandBuffer);

    bool checkbox(const std::string& label, bool& value);
    bool checkbox(const std::string& label, int& value);
    bool combo(const std::string& label, int& value, const std::vector<std::string>& items);
    bool sliderInt(const std::string& label, int& value, int min, int max);
    bool colorPicker4(const std::string& label, glm::vec4& value);
    bool sliderFloat(const std::string& label, float& value, float min, float max);
    bool button(const std::string& label);

    template <typename... Args>
    void text(const char* fmt, const Args&... args) {
        ImGui::Text(fmt, args...);
    }
};
