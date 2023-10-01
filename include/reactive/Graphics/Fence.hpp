#pragma once

namespace rv {
class Context;

struct FenceCreateInfo {
    bool signaled = true;
};

class Fence {
public:
    Fence(const Context* context, FenceCreateInfo createInfo);

    auto getFence() const -> vk::Fence { return *fence; }

    void wait() const;
    void reset() const;
    auto finished() const -> bool;

private:
    const Context* context = nullptr;

    vk::UniqueFence fence;
};
}  // namespace rv
