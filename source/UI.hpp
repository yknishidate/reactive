#include "Vulkan/Vulkan.hpp"
#include <glm/glm.hpp>

class UI
{
public:
    static void Init();
    static void StartFrame();
    static void Prepare();
    static void Render(vk::CommandBuffer commandBuffer);

    static bool Checkbox(const std::string& label, bool& value);
    static bool Combo(const std::string& label, int& value, const std::vector<std::string>& items);
    static bool SliderInt(const std::string& label, int& value, int min, int max);
    static bool ColorPicker4(const std::string& label, glm::vec4& value);
};
