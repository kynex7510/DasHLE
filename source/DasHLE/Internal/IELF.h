#ifndef _DASHLE_INTERNAL_IELF_H
#define _DASHLE_INTERNAL_IELF_H

#include "DasHLE/Base.h"

#include <span>
#include <vector>
#include <string>
#include <algorithm>

#define IELF_ASSERT_CONFIG(cfg) static_assert(::dashle::internal::ielf::ConfigType<cfg>)

namespace dashle::internal::ielf {

namespace constants {

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
constexpr static auto PT_DYNAMIC = 2;

constexpr static auto DT_NULL = 0;
constexpr static auto DT_PLTRELSZ = 2;
constexpr static auto DT_STRTAB = 5;
constexpr static auto DT_SYMTAB	= 6;
constexpr static auto DT_RELA = 7;
constexpr static auto DT_RELASZ = 8;
constexpr static auto DT_RELAENT = 9;
constexpr static auto DT_REL = 17;
constexpr static auto DT_RELSZ = 18;
constexpr static auto DT_RELENT	= 19;
constexpr static auto DT_PLTREL	= 20;
constexpr static auto DT_JMPREL	= 23;

constexpr static auto SHT_LOPROC = 0x70000000;
constexpr static auto SHT_ARM_ATTRIBUTES = SHT_LOPROC + 3;

constexpr static auto STN_UNDEF	= 0;

constexpr static auto PF_X = (1 << 0);
constexpr static auto PF_W = (1 << 1);
constexpr static auto PF_R = (1 << 2);

} // namespace dashle::internal::ielf::constants

using namespace constants;

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

struct Elf32_Dyn {
    Elf32_Sword d_tag;
    union {
        Elf32_Word d_val;
        Elf32_Addr d_ptr;
    } d_un;
};

static_assert(sizeof(Elf32_Dyn) == 0x08);

struct Elf64_Dyn {
    Elf64_Sxword d_tag;
    union {
        Elf64_Xword d_val;
        Elf64_Addr d_ptr;
    } d_un;
};

static_assert(sizeof(Elf64_Dyn) == 0x10);

struct Elf32_Sym {
    Elf32_Word st_name;
    Elf32_Addr st_value;
    Elf32_Word st_size;
    unsigned char st_info;
    unsigned char st_other;
    Elf32_Section st_shndx;
};

static_assert(sizeof(Elf32_Sym) == 0x10);

struct Elf64_Sym {
  Elf64_Word st_name;
  unsigned char st_info;
  unsigned char st_other;
  Elf64_Section st_shndx;
  Elf64_Addr st_value;
  Elf64_Xword st_size;
};

static_assert(sizeof(Elf64_Sym) == 0x18);

template <typename T>
concept RelConstraint = requires(T a) {
    { a.type() } -> std::convertible_to<usize>;
    { a.symbolIndex() } -> std::convertible_to<usize>;
};

struct Elf32_Rel {
    Elf32_Addr r_offset;
    Elf32_Word r_info;

    Elf32_Word type() const { return r_info & 0xFF; }
    Elf32_Word symbolIndex() const { return r_info >> 8; }
};

static_assert(sizeof(Elf32_Rel) == 0x08 && RelConstraint<Elf32_Rel>);

struct Elf32_Rela : Elf32_Rel {
    Elf32_Sword r_addend;
};

static_assert(sizeof(Elf32_Rela) == 0x0C);

struct Elf64_Rel {
    Elf64_Addr r_offset;
    Elf64_Xword	r_info;

    Elf64_Xword type() const { return r_info & 0xFFFFFFFF; }
    Elf64_Xword symbolIndex() const { return r_info >> 32; }
};

static_assert(sizeof(Elf64_Rel) == 0x10 && RelConstraint<Elf64_Rel>);

struct Elf64_Rela : Elf64_Rel {
    Elf64_Sxword r_addend;
};

static_assert(sizeof(Elf64_Rela) == 0x18);

template <typename T>
concept HalfWordType = std::same_as<T, Elf32_Half> || std::same_as<T, Elf64_Half>;

template <typename T>
concept WordType = std::same_as<T, Elf32_Word> || std::same_as<T, Elf64_Word>;

template <typename T>
concept SwordType = std::same_as<T, Elf32_Sword> || std::same_as<T, Elf64_Sword>;

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
concept DynType = std::same_as<T, Elf32_Dyn> || std::same_as<T, Elf64_Dyn>;

template <typename T>
concept SymType = std::same_as<T, Elf32_Sym> || std::same_as<T, Elf64_Sym>;

template <typename T>
concept RelType = std::same_as<T, Elf32_Rel> || std::same_as<T, Elf64_Rel>;

template <typename T>
concept RelaType = std::same_as<T, Elf32_Rela> || std::same_as<T, Elf64_Rela>;

template <typename T>
concept ConfigType = HalfWordType<typename T::HalfWordType> &&
    WordType<typename T::WordType> &&
    SwordType<typename T::SwordType> &&
    AddrType<typename T::AddrType> &&
    OffsetType<typename T::OffsetType> &&
    HeaderType<typename T::HeaderType> &&
    ProgramHeaderType<typename T::ProgramHeaderType> &&
    SectionHeaderType<typename T::SectionHeaderType> &&
    DynType<typename T::DynType> &&
    SymType<typename T::SymType> &&
    RelType<typename T::RelType> &&
    RelaType<typename T::RelaType> &&
    requires 
{
    { T::ARCH } -> std::convertible_to<typename T::WordType>;
    { T::OBJECT_CLASS } -> std::convertible_to<typename T::WordType>;
    { T::DATA_ENCODING } -> std::convertible_to<typename T::WordType>;
};

template <ConfigType CFG>
using Segments = std::vector<const typename CFG::ProgramHeaderType*>;

// template <ConfigType CFG>
// using Sections = std::vector<const typename CFG::SectionHeaderType*>;

template <ConfigType CFG>
using DynEntries = std::vector<const typename CFG::DynType*>;

struct Config32 {
    using HeaderType = Elf32_Ehdr;
    using ProgramHeaderType = Elf32_Phdr;
    using SectionHeaderType = Elf32_Shdr;
    using DynType = Elf32_Dyn;
    using SymType = Elf32_Sym;
    using HalfWordType = Elf32_Half;
    using WordType = Elf32_Word;
    using SwordType = Elf32_Sword;
    using AddrType = Elf32_Addr;
    using OffsetType = Elf32_Off;
    using RelType = Elf32_Rel;
    using RelaType = Elf32_Rela;
    static constexpr auto OBJECT_CLASS = ELFCLASS32;
};

struct Config64 {
    using HeaderType = Elf64_Ehdr;
    using ProgramHeaderType = Elf64_Phdr;
    using SectionHeaderType = Elf64_Shdr;
    using DynType = Elf64_Dyn;
    using SymType = Elf32_Sym;
    using HalfWordType = Elf64_Half;
    using WordType = Elf64_Word;
    using SwordType = Elf64_Sword;
    using AddrType = Elf64_Addr;
    using OffsetType = Elf64_Off;
    using RelType = Elf64_Rel;
    using RelaType = Elf64_Rela;
    static constexpr auto OBJECT_CLASS = ELFCLASS64;
};

struct ConfigLE {
    static constexpr auto DATA_ENCODING = ELFDATA2LSB;
};

struct ConfigBE {
    static constexpr auto DATA_ENCODING = ELFDATA2MSB;
};

template <ConfigType CFG>
Expected<const typename CFG::HeaderType*> getHeader(const std::span<const u8> buffer) {
    auto header = reinterpret_cast<const CFG::HeaderType*>(buffer.data());

    // Check size.
    if (buffer.size() < sizeof(typename CFG::HeaderType) || buffer.size() < header->e_ehsize)
        return Unexpected(Error::InvalidSize);

    // Check magic.
    if (!std::equal(header->e_ident, header->e_ident + SELFMAG, ELFMAG))
        return Unexpected(Error::InvalidMagic);

    // Check class.
    if (header->e_ident[EI_CLASS] != CFG::OBJECT_CLASS)
        return Unexpected(Error::InvalidClass);

    // Check data encoding.
    if (header->e_ident[EI_DATA] != CFG::DATA_ENCODING)
        return Unexpected(Error::InvalidDataEncoding);

    // Check position indipendent binary.
    if (header->e_type != ET_DYN)
        return Unexpected(Error::NoPIE);

    // Check arch.
    if (header->e_machine != CFG::ARCH)
        return Unexpected(Error::InvalidArch);

    return header;
}

template <ConfigType CFG>
Optional<const typename CFG::ProgramHeaderType*> getProgramHeader(const typename CFG::HeaderType* header) {
    if (header && header->e_phnum) {
        const auto base = reinterpret_cast<uaddr>(header);
        return reinterpret_cast<CFG::ProgramHeaderType*>(base + header->e_phoff);
    }

    return {};
}

template<ConfigType CFG>
Optional<const typename CFG::SectionHeaderType*> getSectionHeader(const typename CFG::HeaderType* header) {
    if (header && header->e_shnum) {
        const auto base = reinterpret_cast<uaddr>(header);
        return reinterpret_cast<CFG::SectionHeaderType*>(base + header->e_shoff);
    }

    return {};
}

template <ConfigType CFG>
Expected<Segments<CFG>> getSegments(const typename CFG::HeaderType* header, typename CFG::WordType type) {
    Segments<CFG> vec;
    DASHLE_TRY_OPTIONAL_CONST(ph, getProgramHeader<CFG>(header), Error::NoSegments);

    for (auto i = 0u; i < header->e_phnum; ++i) {
        if (ph[i].p_type == type)
            vec.push_back(&ph[i]);
    }

    std::sort(vec.begin(), vec.end(), [](const typename CFG::ProgramHeaderType* a, const typename CFG::ProgramHeaderType* b) {
        return a->p_vaddr < b->p_vaddr;
    });
    return vec;
}

template <ConfigType CFG>
uaddr getSegmentAllocBase(const typename CFG::ProgramHeaderType* segment, uaddr base) {
    if (segment->p_align > 1)
        return alignAddr(base + segment->p_vaddr, segment->p_align);

    return segment->p_vaddr;
}

template <ConfigType CFG>
Expected<usize> getSegmentAllocSize(const typename CFG::ProgramHeaderType* segment) {
   if (segment->p_memsz < segment->p_filesz)
    return Unexpected(Error::InvalidSegment);

    if (segment->p_align > 1)
        return alignSize(segment->p_memsz, segment->p_align);

    return segment->p_memsz;
}

/*
template <ConfigType CFG>
Expected<Sections<CFG>> getSections(const typename CFG::HeaderType* header, typename CFG::WordType type) {
    Sections<CFG> vec;
    DASHLE_TRY_OPTIONAL_CONST(sh, getSectionHeader<CFG>(header), Error::NoSections);

    for (auto i = 0u; i < header->e_shnum; ++i) {
        if (sh[i].sh_type == type)
            vec.push_back(&sh[i]);
    }

    return vec;
}
*/

template <ConfigType CFG>
Expected<DynEntries<CFG>> getDynEntries(const typename CFG::HeaderType* header, typename CFG::SwordType tag) {
    DynEntries<CFG> vec;
    const auto base = reinterpret_cast<uaddr>(header);
    DASHLE_TRY_EXPECTED_CONST(dynSegs, getSegments<CFG>(header, PT_DYNAMIC));
    
    for (const auto dyn : dynSegs) {
        auto entry = reinterpret_cast<typename CFG::DynType*>(base + dyn->p_offset);
        while (entry->d_tag != DT_NULL) {
            if (entry->d_tag == tag)
                vec.push_back(entry);

            ++entry;
        }
    }

    return vec;
}

template <ConfigType CFG>
Expected<const typename CFG::DynType*> getDynEntry(const typename CFG::HeaderType* header, typename CFG::SwordType tag) {
    DASHLE_TRY_EXPECTED_CONST(dynEntries, getDynEntries<CFG>(header, tag));
    if (dynEntries.size() != 1)
        return Unexpected(Error::InvalidSize);

    return dynEntries[0];
};

template <ConfigType CFG>
Expected<const typename CFG::SymType*> getSymTab(const typename CFG::HeaderType* header) {
    DASHLE_TRY_EXPECTED_CONST(dynEntry, getDynEntry<CFG>(header, DT_SYMTAB));
    const auto base = reinterpret_cast<uaddr>(header);
    return reinterpret_cast<typename CFG::SymType*>(base + dynEntry->d_un.d_ptr);
}

template <ConfigType CFG>
Expected<std::string> getSymbolName(const typename CFG::HeaderType* header, typename CFG::WordType index) {
    if (index != STN_UNDEF) {
        DASHLE_TRY_EXPECTED_CONST(symtab, getSymTab<CFG>(header));
        DASHLE_TRY_EXPECTED_CONST(strtab, getDynEntry<CFG>(header, DT_STRTAB));
        const auto base = reinterpret_cast<uaddr>(header);
        return reinterpret_cast<const char*>(base + strtab->d_un.d_ptr + symtab[index].st_name);
    }

    return std::string();
}

} // namespace dashle::internal::ielf

#endif /* _DASHLE_INTERNAL_IELF_H */