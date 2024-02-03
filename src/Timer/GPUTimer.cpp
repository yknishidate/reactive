#include "Timer/GPUTimer.hpp"

namespace rv {
GPUTimer::GPUTimer(const Context& _context, const GPUTimerCreateInfo& createInfo)
    : context{&_context} {
    vk::QueryPoolCreateInfo queryPoolInfo;
    queryPoolInfo.setQueryType(vk::QueryType::eTimestamp);
    queryPoolInfo.setQueryCount(2);
    queryPool = context->getDevice().createQueryPoolUnique(queryPoolInfo);
    timestampPeriod = context->getPhysicalDevice().getProperties().limits.timestampPeriod;
}

auto GPUTimer::elapsedInNano() -> float {
    timestamps.fill(0);
    vk::resultCheck(context->getDevice().getQueryPoolResults(
                        *queryPool, 0, 2,
                        timestamps.size() * sizeof(uint64_t),  // dataSize
                        timestamps.data(),                     // pData
                        sizeof(uint64_t),                      // stride
                        vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait),
                    "Failed to get query pool results");
    return timestampPeriod * static_cast<float>(timestamps[1] - timestamps[0]);
}

auto GPUTimer::elapsedInMilli() -> float {
    return elapsedInNano() / 1000000.0f;
}
}  // namespace rv
