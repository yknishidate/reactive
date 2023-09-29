#pragma once
#include <spdlog/spdlog.h>

namespace rv {

#ifdef NDEBUG
#define RV_ASSERT(condition, ...)
#else
#define RV_ASSERT(condition, ...)                                                            \
    if (!(condition)) {                                                                      \
        spdlog::critical("Assertion `" #condition "` failed.\n\t{}\n\tFile: {}\n\tLine: {}", \
                         fmt::format(__VA_ARGS__), __FILE__, __LINE__);                      \
        std::terminate();                                                                    \
    }
#endif
}  // namespace rv
