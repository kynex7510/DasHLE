#ifndef _DASHLE_UTILS_ELF_H
#define _DASHLE_UTILS_ELF_H

#include "DasHLE/Base.h"

#include <span>
#include <vector>

#define ELF_ASSERT_CONFIG(cfg) static_assert(::dashle::utils::elf::ConfigType<cfg>)

namespace dashle::utils::elf {

constexpr static usize EI_NIDENT = 16;
constexpr static usize EI_CLASS = 4;
constexpr static usize EI_DATA = 5;

constexpr static auto ELFMAG = "\177ELF";
constexpr static usize SELFMAG = 4;

constexpr static auto ELFCLASSNONE = 0;
constexpr static auto ELFCLASS32 = 1;
constexpr static auto ELFCLASS64 = 2;

constexpr static auto ELFDATANONE = 0;
constexpr static auto ELFDATA2LSB = 1;
constexpr static auto ELFDATA2MSB = 2;

constexpr static auto ET_DYN = 3;

constexpr static auto EM_NONE = 0;
constexpr static auto EM_ARM = 40;
constexpr static auto EM_AARCH64 = 183;

constexpr static auto PT_LOAD = 1;

constexpr static auto SHT_LOPROC = 0x70000000;
constexpr static auto SHT_ARM_ATTRIBUTES = SHT_LOPROC + 3;

using Elf32_Half = u16;
using Elf32_Word = u32;
using Elf32_Sword = s32;
using Elf32_Xword = u64;
using Elf32_Sxword = s64;
using Elf32_Addr = u32;
using Elf32_Off = u32;
using Elf32_Section = u16;
using Elf32_Versym = Elf32_Half;

using Elf64_Half = u16;
using Elf64_Word = u32;
using Elf64_Sword = s32;
using Elf64_Xword = u64;
using Elf64_Sxword = s64;
using Elf64_Addr = u64;
using Elf64_Off = u64;
using Elf64_Section = u16;
using Elf64_Versym = Elf64_Half;

struct Elf32_Ehdr {
    u8 e_ident[EI_NIDENT];
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry;
    Elf32_Off e_phoff;
    Elf32_Off e_shoff;
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
};

static_assert(sizeof(Elf32_Ehdr) == 0x34);

struct Elf64_Ehdr {
    u8 e_ident[EI_NIDENT];
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry;
    Elf64_Off e_phoff;
    Elf64_Off e_shoff;
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
};

static_assert(sizeof(Elf64_Ehdr) == 0x40);

struct Elf32_Phdr {
    Elf32_Word p_type;
    Elf32_Off p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
};

static_assert(sizeof(Elf32_Phdr) == 0x20);

struct Elf64_Phdr {
    Elf64_Word p_type;
    Elf64_Word p_flags;
    Elf64_Off p_offset;
    Elf64_Addr p_vaddr;
    Elf64_Addr p_paddr;
    Elf64_Xword p_filesz;
    Elf64_Xword p_memsz;
    Elf64_Xword p_align;
};

static_assert(sizeof(Elf64_Phdr) == 0x38);

struct Elf32_Shdr {
    Elf32_Word sh_name;
    Elf32_Word sh_type;
    Elf32_Word sh_flags;
    Elf32_Addr sh_addr;
    Elf32_Off sh_offset;
    Elf32_Word sh_size;
    Elf32_Word sh_link;
    Elf32_Word sh_info;
    Elf32_Word sh_addralign;
    Elf32_Word sh_entsize;
};

static_assert(sizeof(Elf32_Shdr) == 0x28);

struct Elf64_Shdr {
    Elf64_Word sh_name;
    Elf64_Word sh_type;
    Elf64_Xword sh_flags;
    Elf64_Addr sh_addr;
    Elf64_Off sh_offset;
    Elf64_Xword sh_size;
    Elf64_Word sh_link;
    Elf64_Word sh_info;
    Elf64_Xword sh_addralign;
    Elf64_Xword sh_entsize;
};

static_assert(sizeof(Elf64_Shdr) == 0x40);

template <typename T>
concept HalfWordType = std::same_as<T, Elf32_Half> || std::same_as<T, Elf64_Half>;

template <typename T>
concept WordType = std::same_as<T, Elf32_Word> || std::same_as<T, Elf64_Word>;

template <typename T>
concept AddrType = std::same_as<T, Elf32_Addr> || std::same_as<T, Elf64_Addr>;

template <typename T>
concept OffsetType = std::same_as<T, Elf32_Off> || std::same_as<T, Elf64_Off>;

template <typename T>
concept HeaderType = std::same_as<T, Elf32_Ehdr> || std::same_as<T, Elf64_Ehdr>;

template <typename T>
concept ProgramHeaderType = std::same_as<T, Elf32_Phdr> || std::same_as<T, Elf64_Phdr>;

template <typename T>
concept SectionHeaderType = std::same_as<T, Elf32_Shdr> || std::same_as<T, Elf64_Shdr>;

template <typename T>
concept ConfigType = HalfWordType<typename T::HalfWordType> &&
    WordType<typename T::WordType> &&
    AddrType<typename T::AddrType> &&
    OffsetType<typename T::OffsetType> &&
    HeaderType<typename T::HeaderType> &&
    ProgramHeaderType<typename T::ProgramHeaderType> &&
    SectionHeaderType<typename T::SectionHeaderType> &&
    requires 
{
    { T::ARCH } -> std::convertible_to<typename T::WordType>;
    { T::OBJECT_CLASS } -> std::convertible_to<typename T::WordType>;
    { T::DATA_ENCODING } -> std::convertible_to<typename T::WordType>;
};

template <ConfigType CFG>
using Segments = std::vector<const typename CFG::ProgramHeaderType*>;

template <ConfigType CFG>
using Sections = std::vector<const typename CFG::SectionHeaderType*>;

struct Config32 {
    using HeaderType = Elf32_Ehdr;
    using ProgramHeaderType = Elf32_Phdr;
    using SectionHeaderType = Elf32_Shdr;
    using HalfWordType = Elf32_Half;
    using WordType = Elf32_Word;
    using AddrType = Elf32_Addr;
    using OffsetType = Elf32_Off;
    static constexpr auto OBJECT_CLASS = ELFCLASS32;
};

struct Config64 {
    using HeaderType = Elf64_Ehdr;
    using ProgramHeaderType = Elf64_Phdr;
    using SectionHeaderType = Elf64_Shdr;
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

constexpr Elf32_Half detectArch(const std::span<const u8> buffer) {
    if (buffer.size_bytes() >= sizeof(Elf32_Ehdr))
        return reinterpret_cast<const Elf32_Ehdr*>(buffer.data())->e_machine;

    return EM_NONE;
}

constexpr bool isArch(const std::span<const u8> buffer, Elf32_Half arch) {
    return detectArch(buffer) == arch;
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

template<ConfigType CFG>
const CFG::SectionHeaderType* getSectionHeader(const typename CFG::HeaderType* header) {
    if (header && header->e_shnum) {
        const auto base = reinterpret_cast<uaddr>(header);
        return reinterpret_cast<CFG::SectionHeaderType*>(base + header->e_shoff);
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

template <ConfigType CFG>
usize getSegmentsAllocSize(const Segments<CFG>& segments) {
    usize allocSize = 0;

    for (const auto segment : segments) {
        if (segment->p_memsz < segment->p_filesz) {
            allocSize = 0;
            break;
        }

        if (segment->p_align > 1)
            allocSize += alignSize(segment->p_memsz, segment->p_align);
        else
            allocSize += segment->p_memsz;
    }

    return allocSize;
}

template <ConfigType CFG>
Sections<CFG> getSections(const typename CFG::HeaderType* header,
                          const typename CFG::WordType type) {
    Sections<CFG> buffer;
    const auto sh = getSectionHeader<CFG>(header);

    if (sh) {
        for (auto i = 0u; i < header->e_shnum; ++i) {
            if (sh[i].sh_type == type)
                buffer.push_back(&sh[i]);
        }
    }

    return buffer;
}

} // namespace dashle::utils::elf

#endif /* _DASHLE_UTILS_ELF_H */