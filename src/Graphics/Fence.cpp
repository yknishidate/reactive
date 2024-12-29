#include "reactive/Graphics/Fence.hpp"

#include "reactive/Graphics/Context.hpp"

namespace rv {
Fence::Fence(const Context& context, const FenceCreateInfo& createInfo) : m_context{&context} {
    vk::FenceCreateInfo fenceInfo;
    if (createInfo.signaled) {
        fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
    }
    m_fence = m_context->getDevice().createFenceUnique(fenceInfo);
}

void Fence::wait() const {
    if (m_context->getDevice().waitForFences(*m_fence, VK_TRUE, UINT64_MAX) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for m_fence");
    }
}

void Fence::reset() const {
    m_context->getDevice().resetFences(*m_fence);
}

auto Fence::finished() const -> bool {
    return m_context->getDevice().getFenceStatus(*m_fence) == vk::Result::eSuccess;
}
}  // namespace rv
