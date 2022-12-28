#include "Swapchain.hpp"
#include "Context.hpp"
#include "Window/Window.hpp"
#include "Graphics/Image.hpp"
#include "Graphics/Pipeline.hpp"
#include "Scene/Mesh.hpp"
#include "GUI/GUI.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include "GUI/imgui_impl_vulkan_hpp.h"

Swapchain::Swapchain()
    : width{ Window::getWidth() }
    , height{ Window::getHeight() }
{
    uint32_t queueFamily = Context::getQueueFamily();
    swapchain = Context::getDevice().createSwapchainKHRUnique(
        vk::SwapchainCreateInfoKHR()
        .setSurface(Context::getSurface())
        .setMinImageCount(minImageCount)
        .setImageFormat(vk::Format::eB8G8R8A8Unorm)
        .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
        .setImageExtent({ width, height })
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
        .setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
        .setPresentMode(vk::PresentModeKHR::eFifo)
        .setClipped(true)
        .setQueueFamilyIndices(queueFamily));

    // Get images
    swapchainImages = Context::getDevice().getSwapchainImagesKHR(*swapchain);

    // Create image views
    for (auto&& image : swapchainImages) {
        swapchainImageViews.push_back(Context::getDevice().createImageViewUnique(
            vk::ImageViewCreateInfo()
            .setImage(image)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(vk::Format::eB8G8R8A8Unorm)
            .setComponents({ vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA })
            .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 })));
    }

    // Create depth image
    depthImage = Context::getDevice().createImageUnique(
        vk::ImageCreateInfo()
        .setImageType(vk::ImageType::e2D)
        .setFormat(vk::Format::eD32Sfloat)
        .setExtent({ width, height, 1 })
        .setMipLevels(1)
        .setArrayLayers(1)
        .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment));

    vk::MemoryRequirements requirements = Context::getDevice().getImageMemoryRequirements(*depthImage);
    uint32_t memoryTypeIndex = Context::findMemoryTypeIndex(requirements, vk::MemoryPropertyFlagBits::eDeviceLocal);
    depthImageMemory = Context::getDevice().allocateMemoryUnique(
        vk::MemoryAllocateInfo()
        .setAllocationSize(requirements.size)
        .setMemoryTypeIndex(memoryTypeIndex));

    Context::getDevice().bindImageMemory(*depthImage, *depthImageMemory, 0);

    depthImageView = Context::getDevice().createImageViewUnique(
        vk::ImageViewCreateInfo()
        .setImage(*depthImage)
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(vk::Format::eD32Sfloat)
        .setSubresourceRange({ vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 }));

    // Create render pass
    const auto colorAttachment = vk::AttachmentDescription()
        .setFormat(vk::Format::eB8G8R8A8Unorm)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    const auto depthAttachment = vk::AttachmentDescription()
        .setFormat(vk::Format::eD32Sfloat)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    const auto colorAttachmentRef = vk::AttachmentReference()
        .setAttachment(0)
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    const auto depthAttachmentRef = vk::AttachmentReference()
        .setAttachment(1)
        .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    const auto subpass = vk::SubpassDescription()
        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachments(colorAttachmentRef)
        .setPDepthStencilAttachment(&depthAttachmentRef);

    const auto dependency = vk::SubpassDependency()
        .setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

    std::array attachments{ colorAttachment, depthAttachment };

    renderPass = Context::getDevice().createRenderPassUnique(
        vk::RenderPassCreateInfo()
        .setAttachments(attachments)
        .setSubpasses(subpass)
        .setDependencies(dependency));

    size_t imageCount = swapchainImages.size();
    commandBuffers = Context::allocateCommandBuffers(imageCount);
    framebuffers.resize(imageCount);
    fences.resize(imageCount);
    imageAcquiredSemaphore = Context::getDevice().createSemaphoreUnique({});
    renderCompleteSemaphore = Context::getDevice().createSemaphoreUnique({});
    for (uint32_t i = 0; i < imageCount; i++) {
        std::array attachments{ *swapchainImageViews[i], *depthImageView };
        framebuffers[i] = Context::getDevice().createFramebufferUnique(
            vk::FramebufferCreateInfo()
            .setRenderPass(*renderPass)
            .setAttachments(attachments)
            .setWidth(width)
            .setHeight(height)
            .setLayers(1));
        fences[i] = Context::getDevice().createFenceUnique(
            vk::FenceCreateInfo()
            .setFlags(vk::FenceCreateFlagBits::eSignaled));
    }
}

void Swapchain::beginRenderPass() const
{
    const auto renderArea = vk::Rect2D()
        .setExtent({ Window::getWidth(), Window::getHeight() });

    std::array<vk::ClearValue, 2> clearValues;
    clearValues[0].color = { std::array{0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };

    const auto beginInfo = vk::RenderPassBeginInfo()
        .setRenderPass(*renderPass)
        .setClearValues(clearValues)
        .setFramebuffer(*framebuffers[frameIndex])
        .setRenderArea(renderArea);

    commandBuffers[frameIndex]->beginRenderPass(beginInfo, vk::SubpassContents::eInline);
}

void Swapchain::endRenderPass() const
{
    commandBuffers[frameIndex]->endRenderPass();
}

void Swapchain::waitNextFrame()
{
    try {
        frameIndex = Context::getDevice().acquireNextImageKHR(*swapchain, UINT64_MAX, *imageAcquiredSemaphore).value;
    } catch (const std::exception& exception) {
        swapchainRebuild = true;
        spdlog::error(exception.what());
        return;
    }
    vk::Result result = Context::getDevice().waitForFences(*fences[frameIndex], VK_TRUE, UINT64_MAX);
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for fence");
    }
    Context::getDevice().resetFences(*fences[frameIndex]);
}

CommandBuffer Swapchain::beginCommandBuffer()
{
    commandBuffers[frameIndex]->begin(
        vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    return { this, *commandBuffers[frameIndex] };
}

void Swapchain::submit()
{
    commandBuffers[frameIndex]->end();
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    const auto submitInfo = vk::SubmitInfo()
        .setWaitDstStageMask(waitStage)
        .setCommandBuffers(*commandBuffers[frameIndex])
        .setWaitSemaphores(*imageAcquiredSemaphore)
        .setSignalSemaphores(*renderCompleteSemaphore);
    Context::getQueue().submit(submitInfo, *fences[frameIndex]);
}

void Swapchain::present()
{
    if (swapchainRebuild) {
        return;
    }

    const auto presentInfo = vk::PresentInfoKHR()
        .setWaitSemaphores(*renderCompleteSemaphore)
        .setSwapchains(*swapchain)
        .setImageIndices(frameIndex);

    if (Context::getQueue().presentKHR(presentInfo) != vk::Result::eSuccess) {
        swapchainRebuild = true;
        return;
    }
    semaphoreIndex = (semaphoreIndex + 1) % swapchainImages.size();
}

void Swapchain::copyToBackImage(vk::CommandBuffer commandBuffer, const Image& source)
{
    vk::ImageCopy copyRegion;
    copyRegion.setSrcSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });
    copyRegion.setDstSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });
    copyRegion.setExtent({ width, height, 1 });

    vk::Image backImage = getBackImage();
    vk::Image sourceImage = source.getImage();
    Image::setImageLayout(commandBuffer, sourceImage, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal);
    Image::setImageLayout(commandBuffer, backImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

    commandBuffer.copyImage(sourceImage, vk::ImageLayout::eTransferSrcOptimal,
                            backImage, vk::ImageLayout::eTransferDstOptimal, copyRegion);

    Image::setImageLayout(commandBuffer, sourceImage, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eGeneral);
    Image::setImageLayout(commandBuffer, backImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eColorAttachmentOptimal);
}

void Swapchain::clearBackImage(vk::CommandBuffer commandBuffer, std::array<float, 4> color)
{
    Image::setImageLayout(commandBuffer, getBackImage(),
                          vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    commandBuffer.clearColorImage(getBackImage(),
                                  vk::ImageLayout::eTransferDstOptimal,
                                  vk::ClearColorValue{ color },
                                  vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
}
