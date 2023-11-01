#ifndef _DASHLE_BASE_H
#define _DASHLE_BASE_H

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <filesystem>

#define DASHLE_AS_STRING_IMPL(x) #x
#define DASHLE_AS_STRING(x) DASHLE_AS_STRING_IMPL(x)

#define DASHLE_LOG(s) ::dashle::logString(s)

#if defined(NDEBUG)

#define DASHLE_ASSERT(cond) (cond)
#define DASHLE_UNREACHABLE(msg) ::std::abort()

#else

#define DASHLE_ASSERT(cond)                                              \
    do {                                                                 \
        if (!(cond)) {                                                   \
            ::dashle::logString("Assertion failed: " #cond);             \
            ::dashle::logString("In file: " __FILE__);                   \
            ::dashle::logString("On line: " DASHLE_AS_STRING(__LINE__)); \
            ::std::abort();                                              \
        }                                                                \
    } while (false);

#define DASHLE_UNREACHABLE(msg)   \
    do {                          \
        ::dashle::logString(msg); \
        ::std::abort();           \
    } while (false)

#endif // NDEBUG

namespace stdfs = std::filesystem;

namespace dashle {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using usize = std::size_t;
using uaddr = std::uintptr_t;

constexpr static auto IS_64_BIT = sizeof(uaddr) == 8;

constexpr usize alignSize(usize size, usize align) {
    return (size + (align - 1)) & ~(align - 1);
}

void logString(const std::string &s);

} // namespace dashle

#endif /* _DASHLE_BASE_H */