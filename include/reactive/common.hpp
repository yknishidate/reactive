#pragma once
#include <spdlog/spdlog.h>

namespace rv {
constexpr char RED_COLOR[] = "\033[31m";
constexpr char YELLOW_COLOR[] = "\033[33m";
constexpr char RESET_COLOR[] = "\033[0m";

#ifdef NDEBUG
#define RV_ASSERT(condition, ...)
#else
#define RV_ASSERT(condition, ...)                                                           \
    if (!(condition)) {                                                                     \
        spdlog::critical("Assertion {}`" #condition                                         \
                         "`{} failed. {}{}{}\n           File: {}\n           Line: {}",    \
                         YELLOW_COLOR, RESET_COLOR, YELLOW_COLOR, fmt::format(__VA_ARGS__), \
                         RESET_COLOR, __FILE__, __LINE__);                                  \
        std::terminate();                                                                   \
    }
#endif
}  // namespace rv
