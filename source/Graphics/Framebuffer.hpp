#pragma once
#include <vulkan/vulkan.hpp>
#include "ArrayProxy.hpp"
#include "Context.hpp"
#include "RenderPass.hpp"

class Framebuffer {
public:
    Framebuffer(uint32_t width,
                uint32_t height,
                const RenderPass& renderPass,
                ArrayProxy<vk::ImageView> attachments) {
        vk::FramebufferCreateInfo framebufferInfo;
        framebufferInfo.setRenderPass(renderPass.getRenderPass());
        framebufferInfo.setAttachments(attachments);
        framebufferInfo.setWidth(width);
        framebufferInfo.setHeight(height);
        framebufferInfo.setLayers(1);
        framebuffer = Context::getDevice().createFramebufferUnique(framebufferInfo);
    }

    auto getFramebuffer() const { return *framebuffer; }

private:
    vk::UniqueFramebuffer framebuffer;
};
