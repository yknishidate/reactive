#include "Timer/GPUTimer.hpp"

namespace rv {
GPUTimer::GPUTimer(const Context* context, GPUTimerCreateInfo createInfo) : context{context} {
    vk::QueryPoolCreateInfo queryPoolInfo;
    queryPoolInfo.setQueryType(vk::QueryType::eTimestamp);
    queryPoolInfo.setQueryCount(2);
    queryPool = context->getDevice().createQueryPoolUnique(queryPoolInfo);
    timestampPeriod = context->getPhysicalDevice().getProperties().limits.timestampPeriod;
}

auto GPUTimer::elapsedInNano() -> double {
    timestamps.fill(0);
    vk::resultCheck(context->getDevice().getQueryPoolResults(
                        *queryPool, 0, 2,
                        timestamps.size() * sizeof(uint64_t),  // dataSize
                        timestamps.data(),                     // pData
                        sizeof(uint64_t),                      // stride
                        vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait),
                    "Failed to get query pool results");
    return timestampPeriod * static_cast<double>(timestamps[1] - timestamps[0]);
}

auto GPUTimer::elapsedInMilli() -> double {
    return elapsedInNano() / 1000000.0;
}
}  // namespace rv
