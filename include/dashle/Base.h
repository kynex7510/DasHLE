#ifndef _DASHLE_BASE_H
#define _DASHLE_BASE_H

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <filesystem>
#include <expected>
#include <optional>

#define DASHLE_AS_STRING_IMPL(x) #x
#define DASHLE_AS_STRING(x) DASHLE_AS_STRING_IMPL(x)

#define DASHLE_LOG(s) ::dashle::logLine(s)

#if defined(NDEBUG)

#define DASHLE_ASSERT(cond) (cond)
#define DASHLE_UNREACHABLE(msg) ::std::abort()

#else

#define DASHLE_ASSERT(cond)                                     \
    do {                                                        \
        if (!(cond)) {                                          \
            DASHLE_LOG("Assertion failed: " #cond);             \
            DASHLE_LOG("In file: " __FILE__);                   \
            DASHLE_LOG("On line: " DASHLE_AS_STRING(__LINE__)); \
            ::std::abort();                                     \
        }                                                       \
    } while (false);

#define DASHLE_UNREACHABLE(msg) \
    do {                        \
        ::dashle::logLine(msg); \
        ::std::abort();         \
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

enum class Error {
    OpenFailed,
    UnknownArch,
    InvalidObject,
    NoSegments,
    InvalidSegments,
    NoMemory,
    RelocationFailed,
    InvalidSize,
    InvalidAddress,
    NoVirtualMemory,
    NoHostMemory,
    NoMemoryBlock,
};

template <typename T>
using Expected = std::expected<T, Error>;

using Unexpected = std::unexpected<Error>;
using Optional = std::optional<Error>;

constexpr static auto OPTIONAL_SUCCESS = Optional();

constexpr usize alignSize(usize size, usize align) {
    return (size + (align - 1)) & ~(align - 1);
}

void logLine(const std::string& s);
std::string errorAsString(Error error);

} // namespace dashle

#endif /* _DASHLE_BASE_H */