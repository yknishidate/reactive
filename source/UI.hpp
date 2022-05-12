#include "Vulkan/Vulkan.hpp"

class UI
{
public:
    void Init();
    void Render();

private:
    vk::UniqueRenderPass renderPass{};
    vk::UniquePipeline pipeline{};
    vk::ClearValue clearValue{};
    std::vector<vk::UniqueFramebuffer> framebuffers{};
};
