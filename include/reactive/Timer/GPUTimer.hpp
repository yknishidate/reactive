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
    GPUTimer(const Context& context, const GPUTimerCreateInfo& createInfo);

    auto elapsedInNano() -> float;
    auto elapsedInMilli() -> float;

    void start() { m_state = State::Started; }
    void stop() { m_state = State::Stopped; }

private:
    const Context* m_context = nullptr;

    float m_timestampPeriod;

    vk::UniqueQueryPool m_queryPool;

    std::array<uint64_t, 2> m_timestamps{};

    State m_state;
};
}  // namespace rv
