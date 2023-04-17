#pragma once
#include "App.hpp"

class GPUTimer {
public:
    GPUTimer(const App* app);

    void beginTimestamp(vk::CommandBuffer commandBuffer) const;
    void endTimestamp(vk::CommandBuffer commandBuffer) const;
    float elapsedInNano();
    float elapsedInMilli();

private:
    const App* m_app;
    vk::UniqueQueryPool queryPool;
    float timestampPeriod;
};
