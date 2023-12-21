#pragma once
#include "Graphics/Context.hpp"

namespace rv {
struct GPUTimerCreateInfo {};

class GPUTimer {
    friend class CommandBuffer;

public:
    GPUTimer() = default;
    GPUTimer(const Context* context, GPUTimerCreateInfo createInfo);

    auto elapsedInNano() -> float;
    auto elapsedInMilli() -> float;

private:
    const Context* context;

    float timestampPeriod;

    vk::UniqueQueryPool queryPool;

    std::array<uint64_t, 2> timestamps{};
};
}  // namespace rv
