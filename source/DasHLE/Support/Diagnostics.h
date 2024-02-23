#ifndef _DASHLE_SUPPORT_DIAGNOSTICS_H
#define _DASHLE_SUPPORT_DIAGNOSTICS_H

#include <string>
#include <cstdlib>
#include <format>

#define DASHLE_ABORT() ::std::abort()
#define DASHLE_FORMAT(...) ::std::format(__VA_ARGS__)

#if defined(NDEBUG)

#define DASHLE_LOG_LINE(...)
#define DASHLE_ASSERT(cond) (cond)
#define DASHLE_UNREACHABLE(...) DASHLE_ABORT()

#else

#define DASHLE_LOG_LINE(...) ::dashle::_impl::logLine(DASHLE_FORMAT(__VA_ARGS__))

#define DASHLE_ASSERT(cond)                                 \
    do {                                                    \
        if (!(cond)) {                                      \
            DASHLE_LOG_LINE("Assertion failed: {}", #cond); \
            DASHLE_LOG_LINE("In file: {}", __FILE__);       \
            DASHLE_LOG_LINE("On line: {}", __LINE__);       \
            DASHLE_ABORT();                                 \
        }                                                   \
    } while (false)

#define DASHLE_UNREACHABLE(...)       \
    do {                              \
        DASHLE_LOG_LINE(__VA_ARGS__); \
        DASHLE_ABORT();               \
    } while (false)

#endif // NDEBUG

namespace dashle::_impl {
void logLine(const std::string& s);
} // namespace dashle::_impl

#endif /* _DASHLE_SUPPORT_DIAGNOSTICS_H */