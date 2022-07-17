#pragma once
#include <glm/glm.hpp>

namespace GUI
{
    void Init();
    void Shutdown();
    void StartFrame();
    void Render(vk::CommandBuffer commandBuffer);

    bool Checkbox(const std::string& label, bool& value);
    bool Checkbox(const std::string& label, int& value);
    bool Combo(const std::string& label, int& value, const std::vector<std::string>& items);
    bool SliderInt(const std::string& label, int& value, int min, int max);
    bool ColorPicker4(const std::string& label, glm::vec4& value);
    bool SliderFloat(const std::string& label, float& value, float min, float max);
};
