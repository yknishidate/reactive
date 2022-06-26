#include "Swapchain.hpp"
#include "Context.hpp"
#include "Window.hpp"


vk::UniqueSwapchainKHR CreateSwapchain(vk::Device device, vk::SurfaceKHR surface, uint32_t minImageCount, uint32_t queueFamily)
{
    return device.createSwapchainKHRUnique(
        vk::SwapchainCreateInfoKHR()
        .setSurface(surface)
        .setMinImageCount(minImageCount)
        .setImageFormat(vk::Format::eB8G8R8A8Unorm)
        .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
        .setImageExtent({ static_cast<uint32_t>(Window::GetWidth()), static_cast<uint32_t>(Window::GetHeight()) })
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
        .setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
        .setPresentMode(vk::PresentModeKHR::eFifo)
        .setClipped(true)
        .setQueueFamilyIndices(queueFamily)
    );
}

std::vector<vk::UniqueImageView> CreateImageViews(vk::Device device, const std::vector<vk::Image>& images)
{
    std::vector<vk::UniqueImageView> views;
    for (auto&& image : images) {
        views.push_back(device.createImageViewUnique(
            vk::ImageViewCreateInfo()
            .setImage(image)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(vk::Format::eB8G8R8A8Unorm)
            .setComponents({ vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA })
            .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 })
        ));
    }
    return views;
}

vk::UniqueRenderPass CreateRenderPass(vk::Device device)
{
    const auto attachment = vk::AttachmentDescription()
        .setFormat(vk::Format::eB8G8R8A8Unorm)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    const auto colorAttachment = vk::AttachmentReference()
        .setAttachment(0)
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    const auto subpass = vk::SubpassDescription()
        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachments(colorAttachment);

    const auto dependency = vk::SubpassDependency()
        .setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

    return device.createRenderPassUnique(
        vk::RenderPassCreateInfo()
        .setAttachments(attachment)
        .setSubpasses(subpass)
        .setDependencies(dependency)
    );
}

Frame::Frame(vk::RenderPass renderPass, vk::ImageView attachment)
{
    framebuffer = Context::GetDevice().createFramebufferUnique(
        vk::FramebufferCreateInfo()
        .setRenderPass(renderPass)
        .setAttachments(attachment)
        .setWidth(Window::GetWidth())
        .setHeight(Window::GetHeight())
        .setLayers(1)
    );
    commandBuffer = Context::AllocateCommandBuffer();
    fence = Context::GetDevice().createFenceUnique(
        vk::FenceCreateInfo()
        .setFlags(vk::FenceCreateFlagBits::eSignaled)
    );
}

FrameSemaphores::FrameSemaphores()
{
    imageAcquiredSemaphore = Context::GetDevice().createSemaphoreUnique({});
    renderCompleteSemaphore = Context::GetDevice().createSemaphoreUnique({});
}

Swapchain::Swapchain(vk::Device device, vk::SurfaceKHR surface, uint32_t queueFamily)
{
    swapchain = CreateSwapchain(device, surface, minImageCount, queueFamily);
    swapchainImages = device.getSwapchainImagesKHR(*swapchain);
    swapchainImageViews = CreateImageViews(device, swapchainImages);
    renderPass = CreateRenderPass(device);

    size_t imageCount = swapchainImages.size();
    frames = std::vector<Frame>(imageCount);
    frameSemaphores = std::vector<FrameSemaphores>(imageCount);
    for (uint32_t i = 0; i < imageCount; i++) {
        frames[i] = Frame{ *renderPass, *swapchainImageViews[i] };
    }
}

void Swapchain::BeginRenderPass()
{
    const auto renderArea = vk::Rect2D()
        .setExtent({ static_cast<uint32_t>(Window::GetWidth()), static_cast<uint32_t>(Window::GetHeight()) });

    const auto beginInfo = vk::RenderPassBeginInfo()
        .setRenderPass(*renderPass)
        .setFramebuffer(*frames[frameIndex].framebuffer)
        .setRenderArea(renderArea);

    frames[frameIndex].commandBuffer->beginRenderPass(beginInfo, vk::SubpassContents::eInline);
}
