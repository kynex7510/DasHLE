#ifndef _DASHLE_SUPPORT_TYPES_H
#define _DASHLE_SUPPORT_TYPES_H

#include "Poly/Poly.hpp"

#include "DasHLE/Support/Diagnostics.h"

#include <cstdint>
#include <utility>
#include <type_traits>
#include <concepts>
#include <variant>
#include <optional>
#include <expected>

#define DASHLE_TRY_EXPECTED(name, expr)                 \
    auto _##name##Wrapper_ = (expr);                    \
    if (!(_##name##Wrapper_))                           \
        return Unexpected((_##name##Wrapper_).error()); \
    auto name = std::move(_##name##Wrapper_.value())

#define DASHLE_TRY_EXPECTED_CONST(name, expr)              \
    const auto _##name##Wrapper_ = (expr);                 \
    if (!(_##name##Wrapper_))                              \
        return Unexpected((_##name##Wrapper_).error());    \
    const auto name = std::move(_##name##Wrapper_.value())

#define DASHLE_TRY_EXPECTED_VOID(expr)                \
    {                                                 \
        const auto _voidWrapper_ = (expr);            \
        if (!_voidWrapper_)                           \
            return Unexpected(_voidWrapper_.error()); \
    }

#define DASHLE_TRY_OPTIONAL(name, expr, error)       \
    auto _##name##Wrapper_ = (expr);                 \
    if (!(_##name##Wrapper_))                        \
        return Unexpected((error));                  \
    auto name = std::move(_##name##Wrapper_.value())

#define DASHLE_TRY_OPTIONAL_CONST(name, expr, error)       \
    const auto _##name##Wrapper_ = (expr);                 \
    if (!(_##name##Wrapper_))                              \
        return Unexpected((error));                        \
    const auto name = std::move(_##name##Wrapper_.value())

#define DASHLE_ASSERT_WRAPPER(name, expr)            \
    auto _##name##Wrapper_ = (expr);                 \
    DASHLE_ASSERT(_##name##Wrapper_);                \
    auto name = std::move(_##name##Wrapper_.value())

#define DASHLE_ASSERT_WRAPPER_CONST(name, expr)            \
    const auto _##name##Wrapper_ = (expr);                 \
    DASHLE_ASSERT(_##name##Wrapper_);                      \
    const auto name = std::move(_##name##Wrapper_.value())

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

enum class Error {
    OpenFailed,
    InvalidSize,
    InvalidAlignment,
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
    InvalidIndex,
    NoVirtualMemory,
    NoHostMemory,
    NotFound,
    InvalidArgument,
    Duplicate,
    InvalidOperation,
};

enum class GuestVersion {
    Armeabi,     // v5TE
    Armeabi_v7a, // v7
    Arm64_v8a,   // v8
};

template <typename T>
concept Copyable = std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>;

template <typename T>
concept Movable = std::is_move_constructible_v<T> && std::is_move_assignable_v<T>;

template <typename T, typename ... Args>
concept OneOf = (std::same_as<T, Args> || ...);

template <typename Base, typename ... Deriveds>
concept AreDerived = (std::derived_from<Deriveds, Base> && ...);

template <typename T>
using Expected = std::expected<T, Error>;
using Unexpected = std::unexpected<Error>;

template <typename T>
using Optional = std::optional<T>;

template <typename Base, typename ... Deriveds>
using Poly = efl::Poly<Base, Deriveds...>; 

#if defined(NDEBUG)
constexpr static auto DEBUG_MODE = false;
#else
constexpr static auto DEBUG_MODE = true;
#endif // NDEBUG

constexpr static auto BITS_32 = 0b01;
constexpr static auto BITS_64 = 0b10;
constexpr static auto BITS_ANY = BITS_32 | BITS_64;

constexpr static auto EXPECTED_VOID = Expected<void>();

std::string errorAsString(Error error);

} // namespace dashle

template<> 
struct std::formatter<dashle::Error> : std::formatter<std::string> {
    auto format(dashle::Error error, std::format_context& ctx) const {
        return std::formatter<std::string>::format(dashle::errorAsString(error), ctx);
    }
};

#endif /* _DASHLE_SUPPORT_TYPES_H */