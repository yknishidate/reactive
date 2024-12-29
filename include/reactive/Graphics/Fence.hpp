#pragma once

namespace rv {
class Context;

struct FenceCreateInfo {
    bool signaled = true;
};

class Fence {
public:
    Fence(const Context& context, const FenceCreateInfo& createInfo);

    auto getFence() const -> vk::Fence { return *m_fence; }

    void wait() const;
    void reset() const;
    auto finished() const -> bool;

private:
    const Context* m_context = nullptr;

    vk::UniqueFence m_fence;
};
}  // namespace rv
