#include "Vulkan/Vulkan.hpp"
#include <glm/glm.hpp>

class UI
{
public:
    void Init();
    void StartFrame();
    void Prepare();
    void Render(vk::CommandBuffer commandBuffer);

    bool Checkbox(const std::string& label, bool& value);
    bool Combo(const std::string& label, int& value, const std::vector<std::string>& items);
    bool SliderInt(const std::string& label, int& value, int min, int max);
    bool ColorPicker4(const std::string& label, glm::vec4& value);

private:
    vk::UniqueRenderPass renderPass{};
    vk::ClearValue clearValue{};
    std::vector<vk::UniqueFramebuffer> framebuffers{};
};
