#include "reactive/Timer/GPUTimer.hpp"

namespace rv {
GPUTimer::GPUTimer(const Context& _context, const GPUTimerCreateInfo& createInfo)
    : context{&_context} {
    vk::QueryPoolCreateInfo queryPoolInfo;
    queryPoolInfo.setQueryType(vk::QueryType::eTimestamp);
    queryPoolInfo.setQueryCount(2);
    queryPool = context->getDevice().createQueryPoolUnique(queryPoolInfo);
    timestampPeriod = context->getPhysicalDevice().getProperties().limits.timestampPeriod;
    state = State::Ready;
}

auto GPUTimer::elapsedInNano() -> float {
    if (state != State::Stopped) {
        return 0.0f;
    }
    timestamps.fill(0);
    if (context->getDevice().getQueryPoolResults(
            *queryPool, 0, 2,
            timestamps.size() * sizeof(uint64_t),  // dataSize
            timestamps.data(),                     // pData
            sizeof(uint64_t),                      // stride
            vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait) !=
        vk::Result::eSuccess) {
        throw std::runtime_error("Failed to get query pool results");
    }
    state = State::Ready;

    // FIXME: たまに片方に 0 が入っていることがある。同期などの問題が起きているかも。
    return timestampPeriod * static_cast<float>(timestamps[1] - timestamps[0]);
}

auto GPUTimer::elapsedInMilli() -> float {
    return elapsedInNano() / 1000000.0f;
}
}  // namespace rv
