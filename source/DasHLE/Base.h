#ifndef _DASHLE_BASE_H
#define _DASHLE_BASE_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <expected>
#include <optional>
#include <format>

#if defined(NDEBUG)

#define DASHLE_LOG_LINE(...)
#define DASHLE_ASSERT(cond) (cond)
#define DASHLE_UNREACHABLE(...) ::dashle::_internal::breakExec()

#else

#define DASHLE_LOG_LINE(...) ::dashle::_internal::logLine(std::format(__VA_ARGS__))

#define DASHLE_ASSERT(cond)                                 \
    do {                                                    \
        if (!(cond)) {                                      \
            DASHLE_LOG_LINE("Assertion failed: {}", #cond); \
            DASHLE_LOG_LINE("In file: {}", __FILE__);       \
            DASHLE_LOG_LINE("On line: {}", __LINE__);       \
            ::dashle::_internal::breakExec();               \
        }                                                   \
    } while (false)

#define DASHLE_UNREACHABLE(...)           \
    do {                                  \
        DASHLE_LOG_LINE(__VA_ARGS__);     \
        ::dashle::_internal::breakExec(); \
    } while (false)

#endif // NDEBUG

#define DASHLE_TRY_EXPECTED(name, expr)             \
    auto name##Wrapper = (expr);                    \
    if (!(name##Wrapper))                           \
        return Unexpected((name##Wrapper).error()); \
    auto name = name##Wrapper.value()

#define DASHLE_TRY_EXPECTED_CONST(name, expr)       \
    const auto name##Wrapper = (expr);              \
    if (!(name##Wrapper))                           \
        return Unexpected((name##Wrapper).error()); \
    const auto name = name##Wrapper.value()

#define DASHLE_TRY_EXPECTED_VOID(expr)              \
    {                                               \
        const auto voidWrapper = (expr);            \
        if (!voidWrapper)                           \
            return Unexpected(voidWrapper.error()); \
    }

#define DASHLE_TRY_OPTIONAL(name, expr, error) \
    auto name##Wrapper = (expr);               \
    if (!(name##Wrapper))                      \
        return Unexpected((error));            \
    auto name = name##Wrapper.value()

#define DASHLE_TRY_OPTIONAL_CONST(name, expr, error) \
    const auto name##Wrapper = (expr);               \
    if (!(name##Wrapper))                            \
        return Unexpected((error));                  \
    const auto name = name##Wrapper.value()

#define DASHLE_ASSERT_WRAPPER(name, expr) \
    auto name##Wrapper = (expr);          \
    DASHLE_ASSERT(name##Wrapper);         \
    auto name = name##Wrapper.value()

#define DASHLE_ASSERT_WRAPPER_CONST(name, expr)    \
    const auto name##Wrapper = (expr);             \
    DASHLE_ASSERT(name##Wrapper);                  \
    const auto name = name##Wrapper.value()

namespace dashle {

using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using usize = std::size_t;
using uaddr = std::uintptr_t;
using soff = std::ptrdiff_t;

enum class Error {
    OpenFailed,
    InvalidSize,
    InvalidMagic,
    InvalidClass,
    InvalidDataEncoding,
    NoPIE,
    InvalidArch,
    InvalidRelocation,
    NoSegments,
    NoSections,
    InvalidSegment,
    RelocationFailed,
    InvalidAddress,
    NoVirtualMemory,
    NoHostMemory,
    NotFound,
};

template <typename T>
using Expected = std::expected<T, Error>;
using Unexpected = std::unexpected<Error>;

constexpr static auto EXPECTED_VOID = Expected<void>();

template <typename T>
using Optional = std::optional<T>;

#if defined(NDEBUG)
constexpr static auto DEBUG_MODE = false;
#else
constexpr static auto DEBUG_MODE = true;
#endif // NDEBUG

namespace _internal {

[[noreturn]] void breakExec();
void logLine(const std::string& s);

} // namespace dashle::_internal

constexpr uaddr alignAddr(uaddr addr, usize align) {
    return addr & ~(align - 1);
}

constexpr usize alignSize(usize size, usize align) {
    return (size + (align - 1)) & ~(align - 1);
}

std::string errorAsString(Error error);

} // namespace dashle

template<> 
struct std::formatter<dashle::Error> { 
  constexpr auto parse(std::format_parse_context& ctx) { 
    return ctx.begin(); 
  }

  auto format(dashle::Error error, std::format_context& ctx) const { 
    return std::format_to(ctx.out(), "{}", dashle::errorAsString(error)); 
  } 
};

#endif /* _DASHLE_BASE_H */