#pragma once

namespace rv {
class Context;

struct FenceCreateInfo {
    bool signaled = true;
};

class Fence {
public:
    Fence(const Context* context, FenceCreateInfo createInfo);

    void wait() const;

    auto finished() const -> bool;

    void reset() const;

private:
    const Context* context = nullptr;

    vk::UniqueFence fence;
};
}  // namespace rv
