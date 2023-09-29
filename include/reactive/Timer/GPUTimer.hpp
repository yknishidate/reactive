#pragma once
#include "Graphics/Context.hpp"

namespace rv {
struct GPUTimerCreateInfo {};

class GPUTimer {
    friend class CommandBuffer;

public:
    GPUTimer() = default;
    GPUTimer(const Context* context, GPUTimerCreateInfo createInfo);

    double elapsedInNano();
    double elapsedInMilli();

    std::array<uint64_t, 2> timestamps;

private:
    const Context* context;
    vk::UniqueQueryPool queryPool;
    float timestampPeriod;
};
}  // namespace rv
