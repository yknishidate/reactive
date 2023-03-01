#pragma once
#include <vulkan/vulkan.hpp>
#include "ArrayProxy.hpp"
#include "Context.hpp"
#include "Image.hpp"

class RenderPass {
public:
    RenderPass() = default;
    RenderPass(uint32_t width,
               uint32_t height,
               ArrayProxy<Image> colorImages,
               const Image& depthImage) {
        std::vector<vk::AttachmentDescription> attachmentDescs{};
        std::vector<vk::AttachmentReference> colorAttachmentRefs;
        int index = 0;
        for (auto& image : colorImages) {
            attachmentDescs.push_back(image.createAttachmentDesc());
            clearValues.push_back({std::array{0.0f, 0.0f, 0.0f, 1.0f}});
            colorAttachmentRefs.push_back(image.createAttachmentRef(index));
            index++;
        }
        attachmentDescs.push_back(depthImage.createAttachmentDesc());
        clearValues.push_back({{1.0f, 0}});

        vk::AttachmentReference depthAttachmentRef = depthImage.createAttachmentRef(index);

        renderArea.setExtent({width, height});
        createRenderPass(attachmentDescs, colorAttachmentRefs, depthAttachmentRef);
    }

    RenderPass(uint32_t width,
               uint32_t height,
               ArrayProxy<vk::AttachmentDescription> attachmentDescs,
               ArrayProxy<vk::AttachmentReference> colorAttachmentRefs,
               vk::AttachmentReference depthAttachmentRef) {
        renderArea.setExtent({width, height});
        for (size_t i = 0; i < attachmentDescs.size(); i++) {
            clearValues.push_back({std::array{0.0f, 0.0f, 0.0f, 1.0f}});
        }
        clearValues.push_back({{1.0f, 0}});
        createRenderPass(attachmentDescs, colorAttachmentRefs, depthAttachmentRef);
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
    friend class Swapchain;

    void createRenderPass(ArrayProxy<vk::AttachmentDescription> attachmentDescs,
                          ArrayProxy<vk::AttachmentReference> colorAttachmentRefs,
                          vk::AttachmentReference depthAttachmentRef) {
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

        colorBlendStates.resize(colorAttachmentRefs.size());
        for (auto& state : colorBlendStates) {
            state.setColorWriteMask(
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
        }
    }

    void beginRenderPass(vk::CommandBuffer commandBuffer, vk::Framebuffer framebuffer) const {
        assert(!clearValues.empty());
        vk::RenderPassBeginInfo beginInfo;
        beginInfo.setRenderPass(*renderPass);
        beginInfo.setClearValues(clearValues);
        beginInfo.setFramebuffer(framebuffer);
        beginInfo.setRenderArea(renderArea);
        commandBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);
    }

    void endRenderPass(vk::CommandBuffer commandBuffer) const { commandBuffer.endRenderPass(); }

    vk::UniqueRenderPass renderPass;
    vk::Rect2D renderArea;
    std::vector<vk::ClearValue> clearValues;
    std::vector<vk::PipelineColorBlendAttachmentState> colorBlendStates;
};
