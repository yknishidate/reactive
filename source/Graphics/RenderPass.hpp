#pragma once
#include <vulkan/vulkan.hpp>
#include "ArrayProxy.hpp"
#include "Context.hpp"

class RenderPass {
public:
    RenderPass(uint32_t width,
               uint32_t height,
               ArrayProxy<vk::AttachmentDescription> attachmentDescs,
               ArrayProxy<vk::AttachmentReference> colorAttachmentRefs,
               vk::AttachmentReference depthAttachmentRef,
               ArrayProxy<vk::ImageView> attachments) {
        renderArea.setExtent({width, height});

        vk::SubpassDescription subpass;
        subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
        subpass.setColorAttachments(colorAttachmentRefs);
        subpass.setPDepthStencilAttachment(&depthAttachmentRef);

        vk::SubpassDependency dependency;
        dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL);
        dependency.setDstSubpass(0);
        dependency.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput |
                                   vk::PipelineStageFlagBits::eEarlyFragmentTests);
        dependency.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput |
                                   vk::PipelineStageFlagBits::eEarlyFragmentTests);
        dependency.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite |
                                    vk::AccessFlagBits::eDepthStencilAttachmentWrite);

        vk::RenderPassCreateInfo renderPassInfo;
        renderPassInfo.setAttachments(attachmentDescs);
        renderPassInfo.setSubpasses(subpass);
        renderPassInfo.setDependencies(dependency);
        renderPass = Context::getDevice().createRenderPassUnique(renderPassInfo);

        vk::FramebufferCreateInfo framebufferInfo;
        framebufferInfo.setRenderPass(*renderPass);
        framebufferInfo.setAttachments(attachments);
        framebufferInfo.setWidth(width);
        framebufferInfo.setHeight(height);
        framebufferInfo.setLayers(1);
        framebuffer = Context::getDevice().createFramebufferUnique(framebufferInfo);
    }

    void setClearValues(const std::vector<vk::ClearValue>& values) { clearValues = values; }

    auto getRenderPass() const { return *renderPass; }

private:
    friend class CommandBuffer;

    void beginRenderPass(vk::CommandBuffer commandBuffer) const {
        assert(!clearValues.empty());
        vk::RenderPassBeginInfo beginInfo;
        beginInfo.setRenderPass(*renderPass);
        beginInfo.setClearValues(clearValues);
        beginInfo.setFramebuffer(*framebuffer);
        beginInfo.setRenderArea(renderArea);
        commandBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);
    }

    void endRenderPass(vk::CommandBuffer commandBuffer) const { commandBuffer.endRenderPass(); }

    vk::UniqueRenderPass renderPass;
    vk::UniqueFramebuffer framebuffer;
    vk::Rect2D renderArea;
    std::vector<vk::ClearValue> clearValues;
};
