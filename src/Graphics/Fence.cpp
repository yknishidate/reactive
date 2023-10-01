#include "Graphics/Fence.hpp"

#include "Graphics/Context.hpp"

namespace rv {
Fence::Fence(const Context* context, FenceCreateInfo createInfo) : context{context} {
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

auto Fence::finished() const -> bool {
    return context->getDevice().getFenceStatus(*fence) == vk::Result::eSuccess;
}

void Fence::reset() const {
    context->getDevice().resetFences(*fence);
}
}  // namespace rv
