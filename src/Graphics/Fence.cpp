#include "Graphics/Fence.hpp"

#include "Graphics/Context.hpp"

namespace rv {
Fence::Fence(const Context& _context, const FenceCreateInfo& createInfo) : context{&_context} {
    vk::FenceCreateInfo fenceInfo;
    if (createInfo.signaled) {
        fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
    }
    fence = context->getDevice().createFenceUnique(fenceInfo);
}

void Fence::wait() const {
    vk::resultCheck(context->getDevice().waitForFences(*fence, VK_TRUE, UINT64_MAX),
                    "Failed to wait for fence");
}

void Fence::reset() const {
    context->getDevice().resetFences(*fence);
}

auto Fence::finished() const -> bool {
    return context->getDevice().getFenceStatus(*fence) == vk::Result::eSuccess;
}
}  // namespace rv
