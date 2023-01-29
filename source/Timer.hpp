#pragma once
#include <chrono>

class Timer {
public:
    Timer() { restart(); }

    void restart() { startTime = std::chrono::steady_clock::now(); }

    double elapsedNS() const {
        auto endTime = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count();
    }

private:
    std::chrono::time_point<std::chrono::steady_clock> startTime;
};
