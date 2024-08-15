#pragma once
#include "reactive/Graphics/Context.hpp"

namespace rv {
struct GPUTimerCreateInfo {};

class GPUTimer {
    friend class CommandBuffer;

    enum class State {
        Ready,
        Started,
        Stopped,
    };

public:
    GPUTimer() = default;
    GPUTimer(const Context& _context, const GPUTimerCreateInfo& createInfo);

    auto elapsedInNano() -> float;
    auto elapsedInMilli() -> float;

    void start() { state = State::Started; }
    void stop() { state = State::Stopped; }

private:
    const Context* context = nullptr;

    float timestampPeriod;

    vk::UniqueQueryPool queryPool;

    std::array<uint64_t, 2> timestamps{};

    State state;
};
}  // namespace rv
