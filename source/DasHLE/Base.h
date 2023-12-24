#ifndef _DASHLE_BASE_H
#define _DASHLE_BASE_H

#include <cstdint>
#include <expected>
#include <optional>
#include <string>

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
    InvalidArgument
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

std::string errorAsString(Error error);

} // namespace dashle

#endif /* _DASHLE_BASE_H */