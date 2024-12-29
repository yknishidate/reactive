#include "reactive/Timer/GPUTimer.hpp"

namespace rv {
GPUTimer::GPUTimer(const Context& context, const GPUTimerCreateInfo& createInfo)
    : m_context{&context} {
    vk::QueryPoolCreateInfo queryPoolInfo;
    queryPoolInfo.setQueryType(vk::QueryType::eTimestamp);
    queryPoolInfo.setQueryCount(2);
    m_queryPool = m_context->getDevice().createQueryPoolUnique(queryPoolInfo);
    m_timestampPeriod = m_context->getPhysicalDevice().getProperties().limits.timestampPeriod;
    m_state = State::Ready;
}

auto GPUTimer::elapsedInNano() -> float {
    if (m_state != State::Stopped) {
        return 0.0f;
    }
    m_timestamps.fill(0);
    if (m_context->getDevice().getQueryPoolResults(
            *m_queryPool, 0, 2,
            m_timestamps.size() * sizeof(uint64_t),  // dataSize
            m_timestamps.data(),                     // pData
            sizeof(uint64_t),                      // stride
            vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait) !=
        vk::Result::eSuccess) {
        throw std::runtime_error("Failed to get query pool results");
    }
    m_state = State::Ready;

    // FIXME: たまに片方に 0 が入っていることがある。同期などの問題が起きているかも。
    return m_timestampPeriod * static_cast<float>(m_timestamps[1] - m_timestamps[0]);
}

auto GPUTimer::elapsedInMilli() -> float {
    return elapsedInNano() / 1000000.0f;
}
}  // namespace rv
