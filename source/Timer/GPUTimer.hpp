#pragma once
#include "Graphics/Context.hpp"

class GPUTimer {
public:
    GPUTimer(const Context* context);

    void beginTimestamp(vk::CommandBuffer commandBuffer) const;
    void endTimestamp(vk::CommandBuffer commandBuffer) const;
    float elapsedInNano();
    float elapsedInMilli();

private:
    const Context* context;
    vk::UniqueQueryPool queryPool;
    float timestampPeriod;
};
