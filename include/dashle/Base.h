#ifndef _DASHLE_BASE_H
#define _DASHLE_BASE_H

#include <cstdint>
#include <cstddef>
#include <filesystem>

// TODO
#define DASHLE_ASSERT(x)

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

} // namespace dashle

#endif /* _DASHLE_BASE_H */