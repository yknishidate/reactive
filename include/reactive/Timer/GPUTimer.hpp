#pragma once
#include "Graphics/Context.hpp"

namespace rv {
struct GPUTimerCreateInfo {};

class GPUTimer {
public:
    GPUTimer() = default;
    GPUTimer(const Context* context, GPUTimerCreateInfo createInfo);

    void beginTimestamp(vk::CommandBuffer commandBuffer) const;
    void endTimestamp(vk::CommandBuffer commandBuffer) const;
    float elapsedInNano();
    float elapsedInMilli();

private:
    const Context* context;
    vk::UniqueQueryPool queryPool;
    float timestampPeriod;
};
}  // namespace rv
