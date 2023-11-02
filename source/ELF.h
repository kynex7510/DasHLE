#ifndef _DASHLE_INTERNAL_ELF_H
#define _DASHLE_INTERNAL_ELF_H

#include "DasHLE.h"

#include <elf.h>

#include <span>
#include <vector>

#define ELF_ASSERT_CONFIG(cfg) static_assert(::dashle::elf::ConfigType<cfg>)

namespace dashle::elf {

template <typename T>
concept HeaderType = std::same_as<T, Elf32_Ehdr> || std::same_as<T, Elf64_Ehdr>;

template <typename T>
concept ProgramHeaderType = std::same_as<T, Elf32_Phdr> || std::same_as<T, Elf64_Phdr>;

template <typename T>
concept HalfWordType = std::same_as<T, Elf32_Half> || std::same_as<T, Elf64_Half>;

template <typename T>
concept WordType = std::same_as<T, Elf32_Word> || std::same_as<T, Elf64_Word>;

template <typename T>
concept AddrType = std::same_as<T, Elf32_Addr> || std::same_as<T, Elf64_Addr>;

template <typename T>
concept OffsetType = std::same_as<T, Elf32_Off> || std::same_as<T, Elf64_Off>;

template <typename T>
concept ConfigType = HeaderType<typename T::HeaderType> &&
    ProgramHeaderType<typename T::ProgramHeaderType> &&
    HalfWordType<typename T::HalfWordType> &&
    WordType<typename T::WordType> &&
    AddrType<typename T::AddrType> &&
    OffsetType<typename T::OffsetType> &&
   requires(T)
{
    { T::ARCH } -> std::convertible_to<typename T::WordType>;
    { T::OBJECT_CLASS } -> std::convertible_to<typename T::WordType>;
    { T::DATA_ENCODING } -> std::convertible_to<typename T::WordType>;
};

template <ConfigType CFG>
using Segments = std::vector<const typename CFG::ProgramHeaderType*>;

struct Config32 {
    using HeaderType = Elf32_Ehdr;
    using ProgramHeaderType = Elf32_Phdr;
    using HalfWordType = Elf32_Half;
    using WordType = Elf32_Word;
    using AddrType = Elf32_Addr;
    using OffsetType = Elf32_Off;
    static constexpr auto OBJECT_CLASS = ELFCLASS32;
};

struct Config64 {
    using HeaderType = Elf64_Ehdr;
    using ProgramHeaderType = Elf64_Phdr;
    using HalfWordType = Elf64_Half;
    using WordType = Elf64_Word;
    using AddrType = Elf64_Addr;
    using OffsetType = Elf64_Off;
    static constexpr auto OBJECT_CLASS = ELFCLASS64;
};

struct ConfigLE {
    static constexpr auto DATA_ENCODING = ELFDATA2LSB;
};

struct ConfigBE {
    static constexpr auto DATA_ENCODING = ELFDATA2MSB;
};

constexpr inline usize detectArch(const std::span<const u8> buffer) {
    if (buffer.size_bytes() >= sizeof(Elf32_Ehdr))
        return reinterpret_cast<const Elf32_Ehdr*>(buffer.data())->e_machine;

    return EM_NUM;
}

template <ConfigType CFG>
const CFG::HeaderType* getHeader(const std::span<const u8> buffer) {
    auto header = reinterpret_cast<const CFG::HeaderType*>(buffer.data());

    // Check size.
    if (buffer.size() < sizeof(typename CFG::HeaderType) || buffer.size() < header->e_ehsize)
        return nullptr;

    // Check magic.
    if (!std::equal(header->e_ident, header->e_ident + SELFMAG, ELFMAG))
        return nullptr;

    // Check class.
    if (header->e_ident[EI_CLASS] != CFG::OBJECT_CLASS)
        return nullptr;

    // Check data encoding.
    if (header->e_ident[EI_DATA] != CFG::DATA_ENCODING)
        return nullptr;

    // Check position indipendent binary.
    if (header->e_type != ET_DYN)
        return nullptr;

    // Check arch.
    if (header->e_machine != CFG::ARCH)
        return nullptr;

    return header;
}

template <ConfigType CFG>
const CFG::ProgramHeaderType* getProgramHeader(const typename CFG::HeaderType* header) {
    if (header && header->e_phnum) {
        const auto base = reinterpret_cast<uaddr>(header);
        return reinterpret_cast<CFG::ProgramHeaderType*>(base + header->e_phoff);
    }

    return nullptr;
}

template <ConfigType CFG>
Segments<CFG> getSegments(const typename CFG::HeaderType* header,
                          const typename CFG::WordType type) {
    Segments<CFG> buffer;
    const auto ph = getProgramHeader<CFG>(header);

    if (ph) {
        for (auto i = 0u; i < header->e_phnum; ++i) {
            if (ph[i].p_type == type)
                buffer.push_back(&ph[i]);
        }
    }

    return buffer;
}

} // namespace dashle::elf

#endif /* _DASHLE_INTERNAL_ELF_H */