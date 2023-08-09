#include "Timer/GPUTimer.hpp"

namespace rv {
GPUTimer::GPUTimer(const Context* context, GPUTimerCreateInfo createInfo) : context{context} {
    vk::QueryPoolCreateInfo queryPoolInfo;
    queryPoolInfo.setQueryType(vk::QueryType::eTimestamp);
    queryPoolInfo.setQueryCount(2);
    queryPool = context->getDevice().createQueryPoolUnique(queryPoolInfo);
    timestampPeriod = context->getPhysicalDevice().getProperties().limits.timestampPeriod;
}

void GPUTimer::beginTimestamp(vk::CommandBuffer commandBuffer) const {
    commandBuffer.resetQueryPool(*queryPool, 0, 2);
    commandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, *queryPool, 0);
}

void GPUTimer::endTimestamp(vk::CommandBuffer commandBuffer) const {
    commandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, *queryPool, 1);
}

float GPUTimer::elapsedInNano() {
    std::array<uint64_t, 2> timestamps{};
    vk::resultCheck(context->getDevice().getQueryPoolResults(
                        *queryPool, 0, 2,
                        timestamps.size() * sizeof(uint64_t),  // dataSize
                        timestamps.data(),                     // pData
                        sizeof(uint64_t),                      // stride
                        vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait),
                    "Failed to get query pool results.");
    return timestampPeriod * static_cast<float>(timestamps[1] - timestamps[0]);
}

float GPUTimer::elapsedInMilli() {
    return elapsedInNano() / 1000000.0f;
}
}  // namespace rv
