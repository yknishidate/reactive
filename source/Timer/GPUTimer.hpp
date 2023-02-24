#pragma once

class GPUTimer {
public:
    GPUTimer();

    void beginTimestamp(vk::CommandBuffer commandBuffer) const;
    void endTimestamp(vk::CommandBuffer commandBuffer) const;
    float elapsedInNano();
    float elapsedInMilli();

private:
    vk::UniqueQueryPool queryPool;
    float timestampPeriod;
};
