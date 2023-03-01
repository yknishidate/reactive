#pragma once
#include <vulkan/vulkan.hpp>
#include "ArrayProxy.hpp"
#include "Context.hpp"

class RenderPass {
public:
    RenderPass(uint32_t width,
               uint32_t height,
               ArrayProxy<Image> colorImages,
               const Image& depthImage) {
        renderArea.setExtent({width, height});

        std::vector<vk::AttachmentDescription> attachmentDescs{};
        std::vector<vk::ImageView> attachments;
        std::vector<vk::AttachmentReference> colorAttachmentRefs;
        int index = 0;
        for (auto& image : colorImages) {
            attachmentDescs.push_back(image.createAttachmentDesc());
            attachments.push_back(image.getView());
            clearValues.push_back({std::array{0.0f, 0.0f, 0.0f, 1.0f}});
            colorAttachmentRefs.push_back(image.createAttachmentRef(index));
            index++;
        }
        attachmentDescs.push_back(depthImage.createAttachmentDesc());
        attachments.push_back(depthImage.getView());
        clearValues.push_back({{1.0f, 0}});

        vk::AttachmentReference depthAttachmentRef = depthImage.createAttachmentRef(index);

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

        colorBlendStates.resize(colorAttachmentRefs.size());
        for (auto& state : colorBlendStates) {
            state.setColorWriteMask(
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
        }
    }

    auto getRenderPass() const { return *renderPass; }

    vk::PipelineColorBlendStateCreateInfo createColorBlending() const {
        vk::PipelineColorBlendStateCreateInfo colorBlending;
        colorBlending.setAttachments(colorBlendStates);
        colorBlending.setLogicOpEnable(VK_FALSE);
        return colorBlending;
    }

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
    std::vector<vk::PipelineColorBlendAttachmentState> colorBlendStates;
};
