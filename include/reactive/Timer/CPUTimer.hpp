#pragma once
#include <chrono>

namespace rv {
class CPUTimer {
public:
    CPUTimer() { restart(); }

    void restart() { startTime = std::chrono::steady_clock::now(); }

    auto elapsedInNano() const -> double {
        auto endTime = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count();
    }

    auto elapsedInMilli() const -> double { return elapsedInNano() / 1000000.0; }

private:
    std::chrono::time_point<std::chrono::steady_clock> startTime;
};
}  // namespace rv
