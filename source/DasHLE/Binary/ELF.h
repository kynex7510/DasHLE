#ifndef _DASHLE_BINARY_ELF_H
#define _DASHLE_BINARY_ELF_H

#include "DasHLE/Host/Memory.h"

#include <span>
#include <vector>
#include <string>
#include <algorithm>

namespace dashle::binary::elf {

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
constexpr static auto DT_INIT_ARRAY	= 25;
constexpr static auto DT_FINI_ARRAY	= 26;
constexpr static auto DT_INIT_ARRAYSZ = 27;
constexpr static auto DT_FINI_ARRAYSZ = 28;

constexpr static auto SHT_LOPROC = 0x70000000;
constexpr static auto SHT_ARM_ATTRIBUTES = SHT_LOPROC + 3;

constexpr static auto STN_UNDEF	= 0;

constexpr static auto PF_X = (1 << 0);
constexpr static auto PF_W = (1 << 1);
constexpr static auto PF_R = (1 << 2);

constexpr static auto R_ARM_ABS32 = 2;
constexpr static auto R_ARM_GLOB_DAT = 21;
constexpr static auto R_ARM_JUMP_SLOT = 22;
constexpr static auto R_ARM_RELATIVE = 23;
constexpr static auto R_AARCH64_ABS64 = 257;
constexpr static auto R_AARCH64_GLOB_DAT = 1025;
constexpr static auto R_AARCH64_JUMP_SLOT = 1026;
constexpr static auto R_AARCH64_RELATIVE = 1027;

/* Raw types */

using Half = u16;
using Word = u32;
using Sword = s32;
using Xword = u64;
using Sxword = s64;
using Addr32 = u32;
using Addr64 = u64;
using Off32 = u32;
using Off64 = u64;
using Section = u16;
using Versym = Half;

struct Ehdr32 {
    u8 e_ident[EI_NIDENT];
    Half e_type;
    Half e_machine;
    Word e_version;
    Addr32 e_entry;
    Off32 e_phoff;
    Off32 e_shoff;
    Word e_flags;
    Half e_ehsize;
    Half e_phentsize;
    Half e_phnum;
    Half e_shentsize;
    Half e_shnum;
    Half e_shstrndx;
};

static_assert(sizeof(Ehdr32) == 0x34);

struct Ehdr64 {
    u8 e_ident[EI_NIDENT];
    Half e_type;
    Half e_machine;
    Word e_version;
    Addr64 e_entry;
    Off64 e_phoff;
    Off64 e_shoff;
    Word e_flags;
    Half e_ehsize;
    Half e_phentsize;
    Half e_phnum;
    Half e_shentsize;
    Half e_shnum;
    Half e_shstrndx;
};

static_assert(sizeof(Ehdr64) == 0x40);

struct Shdr32 {
    Word sh_name;
    Word sh_type;
    Word sh_flags;
    Addr32 sh_addr;
    Off32 sh_offset;
    Word sh_size;
    Word sh_link;
    Word sh_info;
    Word sh_addralign;
    Word sh_entsize;
};

static_assert(sizeof(Shdr32) == 0x28);

struct Shdr64 {
    Word sh_name;
    Word sh_type;
    Xword sh_flags;
    Addr64 sh_addr;
    Off64 sh_offset;
    Xword sh_size;
    Word sh_link;
    Word sh_info;
    Xword sh_addralign;
    Xword sh_entsize;
};

static_assert(sizeof(Shdr64) == 0x40);

struct Phdr32 {
    Word p_type;
    Off32 p_offset;
    Addr32 p_vaddr;
    Addr32 p_paddr;
    Word p_filesz;
    Word p_memsz;
    Word p_flags;
    Word p_align;
};

static_assert(sizeof(Phdr32) == 0x20);

struct Phdr64 {
    Word p_type;
    Word p_flags;
    Off64 p_offset;
    Addr64 p_vaddr;
    Addr64 p_paddr;
    Xword p_filesz;
    Xword p_memsz;
    Xword p_align;
};

static_assert(sizeof(Phdr64) == 0x38);

struct Dyn32 {
    Sword d_tag;
    union {
        Word d_val;
        Addr32 d_ptr;
    } d_un;
};

static_assert(sizeof(Dyn32) == 0x08);

struct Dyn64 {
    Sxword d_tag;
    union {
        Xword d_val;
        Addr64 d_ptr;
    } d_un;
};

static_assert(sizeof(Dyn64) == 0x10);

struct Sym32 {
    Word st_name;
    Addr32 st_value;
    Word st_size;
    u8 st_info;
    u8 st_other;
    Section st_shndx;
};

static_assert(sizeof(Sym32) == 0x10);

struct Sym64 {
  Word st_name;
  u8 st_info;
  u8 st_other;
  Section st_shndx;
  Addr64 st_value;
  Xword st_size;
};

static_assert(sizeof(Sym64) == 0x18);

template <typename T>
concept RelConstraint = requires(T a) {
    { a.type() } -> std::convertible_to<usize>;
    { a.symbolIndex() } -> std::convertible_to<usize>;
};

struct Rel32 {
    Addr32 r_offset;
    Word r_info;

    Word type() const { return r_info & 0xFF; }
    Word symbolIndex() const { return r_info >> 8; }
};

static_assert(sizeof(Rel32) == 0x08 && RelConstraint<Rel32>);

struct Rela32 : Rel32 {
    Sword r_addend;
};

static_assert(sizeof(Rela32) == 0x0C);

struct Rel64 {
    Addr64 r_offset;
    Xword r_info;

    Xword type() const { return r_info & 0xFFFFFFFF; }
    Xword symbolIndex() const { return r_info >> 32; }
};

static_assert(sizeof(Rel64) == 0x10 && RelConstraint<Rel64>);

struct Rela64 : Rel64 {
    Sxword r_addend;
};

static_assert(sizeof(Rela64) == 0x18);

/* Wrappers */

struct IHeader {
    virtual const u8* ident() const = 0;
    virtual Half type() const = 0;
    virtual Half machine() const = 0;
    virtual Word version() const = 0;
    virtual Addr64 entry() const = 0;
    virtual Off64 phoff() const = 0;
    virtual Off64 shoff() const = 0;
    virtual Word flags() const = 0;
    virtual Half ehsize() const = 0;
    virtual Half phentsize() const = 0;
    virtual Half phnum() const = 0;
    virtual Half shentsize() const = 0;
    virtual Half shnum() const = 0;
    virtual Half shstrndx() const = 0;
};

struct ISectionHeader {
    virtual Word name() const = 0;
    virtual Word type() const = 0;
    virtual Xword flags() const = 0;
    virtual Addr64 addr() const = 0;
    virtual Off64 offset() const = 0;
    virtual Xword size() const = 0;
    virtual Word link() const = 0;
    virtual Word info() const = 0;
    virtual Xword addralign() const = 0;
    virtual Xword entsize() const = 0;
};

struct IProgramHeader {
    virtual Word type() const = 0;
    virtual Word flags() const = 0;
    virtual Off64 offset() const = 0;
    virtual Addr64 vaddr() const = 0;
    virtual Addr64 paddr() const = 0;
    virtual Xword filesz() const = 0;
    virtual Xword memsz() const = 0;
    virtual Xword align() const = 0;

    Expected<uaddr> allocationOffset() const;
    Expected<usize> allocationSize() const;
};

struct IDynEntry {
    virtual Sxword tag() const = 0;
    virtual Xword val() const = 0;
    virtual Addr64 ptr() const = 0;
};

struct ISymEntry {
    virtual Word name() const = 0;
    virtual Addr64 value() const = 0;
    virtual Xword size() const = 0;
    virtual u8 info() const = 0;
    virtual u8 other() const = 0;
    virtual Section shndx() const = 0;
};

namespace _impl {

template <typename T>
requires (OneOf<T, Ehdr32, Ehdr64>)
class HeaderImpl final : public IHeader {
    const T* m_Ptr = nullptr;

    const u8* ident() const override { return m_Ptr->e_ident; }
    Half type() const override { return m_Ptr->e_type; }
    Half machine() const override { return m_Ptr->e_machine; }
    Word version() const override { return m_Ptr->e_version; }
    Addr64 entry() const override { return m_Ptr->e_entry; }
    Off64 phoff() const override { return m_Ptr->e_phoff; }
    Off64 shoff() const override { return m_Ptr->e_shoff; }
    Word flags() const override { return m_Ptr->e_flags; }
    Half ehsize() const override { return m_Ptr->e_ehsize; }
    Half phentsize() const override { return m_Ptr->e_phentsize; }
    Half phnum() const override { return m_Ptr->e_phnum; }
    Half shentsize() const override { return m_Ptr->e_shentsize; }
    Half shnum() const override { return m_Ptr->e_shnum; }
    Half shstrndx() const override { return m_Ptr->e_shstrndx; }

public:
    HeaderImpl(const T* ptr) : m_Ptr(ptr) { DASHLE_ASSERT(m_Ptr); }
};

template <typename T>
requires (OneOf<T, Shdr32, Shdr64>)
class SectionHeaderImpl final : public ISectionHeader {
    const T* m_Ptr = nullptr;

    Word name() const override { return m_Ptr->sh_name; }
    Word type() const override { return m_Ptr->sh_type; }
    Xword flags() const override { return m_Ptr->sh_flags; }
    Addr64 addr() const override { return m_Ptr->sh_addr; }
    Off64 offset() const override { return m_Ptr->sh_offset; }
    Xword size() const override { return m_Ptr->sh_size; }
    Word link() const override { return m_Ptr->sh_link; }
    Word info() const override { return m_Ptr->sh_info; }
    Xword addralign() const override { return m_Ptr->sh_addralign; }
    Xword entsize() const override { return m_Ptr->sh_entsize; }

public:
    SectionHeaderImpl(const T* ptr) : m_Ptr(ptr) { DASHLE_ASSERT(m_Ptr); }
};

template <typename T>
requires (OneOf<T, Phdr32, Phdr64>)
class ProgramHeaderImpl final : public IProgramHeader {
    const T* m_Ptr = nullptr;

    Word type() const override { return m_Ptr->p_type; }
    Word flags() const override { return m_Ptr->p_flags; }
    Off64 offset() const override { return m_Ptr->p_offset; }
    Addr64 vaddr() const override { return m_Ptr->p_vaddr; }
    Addr64 paddr() const override { return m_Ptr->p_paddr; }
    Xword filesz() const override { return m_Ptr->p_filesz; }
    Xword memsz() const override { return m_Ptr->p_memsz; }
    Xword align() const override { return m_Ptr->p_align; }

public:
    ProgramHeaderImpl(const T* ptr) : m_Ptr(ptr) { DASHLE_ASSERT(m_Ptr); }
};

template <typename T>
requires (OneOf<T, Dyn32, Dyn64>)
class DynEntryImpl : public IDynEntry {
    const T* m_Ptr = nullptr;

    Sxword tag() const override { return m_Ptr->d_tag; }
    Xword val() const override { return m_Ptr->d_un.d_val; }
    Addr64 ptr() const override { return m_Ptr->d_un.d_ptr; }

public:
    DynEntryImpl(const T* ptr) : m_Ptr(ptr) { DASHLE_ASSERT(m_Ptr); }
};

template <typename T>
requires (OneOf<T, Sym32, Sym64>)
struct SymEntryImpl : public ISymEntry {
    const T* m_Ptr = nullptr;

    virtual Word name() const override { return m_Ptr->st_name; }
    virtual Addr64 value() const override { return m_Ptr->st_value; }
    virtual Xword size() const override { return m_Ptr->st_size; }
    virtual u8 info() const override { return m_Ptr->st_info; }
    virtual u8 other() const override { return m_Ptr->st_other; }
    virtual Section shndx() const override { return m_Ptr->st_shndx; }

public:
    SymEntryImpl(const T* ptr) : m_Ptr(ptr) { DASHLE_ASSERT(m_Ptr); }
};

} // namespace dashle::binary::elf::_impl

enum class Version {
    Armeabi,     // v5TE
    Armeabi_v7a, // v7
    Arm64_v8a,   // v8
};

enum class RelocKind {
    Relative,
    Symbol,
};

struct RelocInfo {
    usize patchOffset;
    s64 addend;
    RelocKind kind;
    Optional<std::string> symbolName;
};

struct FuncArrayInfo {
    usize offset;
    usize size;
};

class ELF final {
    using Header32 = _impl::HeaderImpl<Ehdr32>;
    using Header64 = _impl::HeaderImpl<Ehdr64>;
    using SectionHeader32 = _impl::SectionHeaderImpl<Shdr32>;
    using SectionHeader64 = _impl::SectionHeaderImpl<Shdr64>;
    using ProgramHeader32 = _impl::ProgramHeaderImpl<Phdr32>;
    using ProgramHeader64 = _impl::ProgramHeaderImpl<Phdr64>;
    using DynEntry32 = _impl::DynEntryImpl<Dyn32>;
    using DynEntry64 = _impl::DynEntryImpl<Dyn64>;
    using SymEntry32 = _impl::SymEntryImpl<Sym32>;
    using SymEntry64 = _impl::SymEntryImpl<Sym64>;

public:
    using Header = Poly<IHeader, Header32, Header64>;
    using SectionHeader = Poly<ISectionHeader, SectionHeader32, SectionHeader64>;
    using ProgramHeader = Poly<IProgramHeader, ProgramHeader32, ProgramHeader64>;
    using DynEntry = Poly<IDynEntry, DynEntry32, DynEntry64>;
    using SymEntry = Poly<ISymEntry, SymEntry32, SymEntry64>;

private:
    std::vector<u8> m_Buffer;
    Version m_Version = Version::Armeabi_v7a;
    Header m_Header;
    std::vector<SectionHeader> m_SectionHeaders;
    std::vector<ProgramHeader> m_ProgramHeaders;
    DynEntry m_StrTab;
    // TODO: We should load all symbols.
    DynEntry m_SymTab;
    std::vector<RelocInfo> m_Relocs;

    const auto binaryBase() const { return m_Buffer.data(); }

    // TODO: These can be size optimized with a PolymorphicArrayView.
    Expected<void> visitRelArray32(const Rel32* relArray, usize size);

    Expected<void> visitRelArray64(const Rel64* relArray, usize size) {
        return Unexpected(Error::InvalidRelocation);
    }

    Expected<void> visitRelaArray32(const Rela32* relaArray, usize size);

    Expected<void> visitRelaArray64(const Rela64* relaArray, usize size) {
        return Unexpected(Error::InvalidRelocation);
    }

    Expected<void> visitRel();
    Expected<void> visitRela();
    Expected<void> visitJmprel();
    Expected<void> visitRelocs();

public:
    Expected<void> parse(std::vector<u8>&& buffer);
    Version version() const { return m_Version; }
    bool is64Bits() const { return version() == Version::Arm64_v8a; }
    const std::span<const RelocInfo> relocs() const { return m_Relocs; }

    Header header() const { return m_Header; }

    const std::span<const SectionHeader> sectionHeaders() const { return m_SectionHeaders; }
    const std::span<const ProgramHeader> programHeaders() const { return m_ProgramHeaders; }
    Expected<SectionHeader> sectionHeader(usize index) const;
    Expected<ProgramHeader> programHeader(usize index) const;

    Expected<std::vector<SectionHeader>> sectionsOfType(Word type) const;
    Expected<std::vector<ProgramHeader>> segmentsOfType(Word type) const;

    Expected<std::vector<DynEntry>> dynEntriesWithTag(Sword tag) const;
    Expected<DynEntry> dynEntryWithTag(Sword tag) const;

    Expected<SymEntry> symbolByIndex(usize index) const;
    Expected<std::string> stringByOffset(usize offset) const;

    Optional<FuncArrayInfo> initArrayInfo() const;
    Optional<FuncArrayInfo> finiArrayInfo() const;
};

constexpr usize wrapPermissionFlags(Word flags) {
    usize perm = 0;
        
    if (flags & PF_R)
        perm |= host::memory::flags::PERM_READ;

    if (flags & PF_W)
        perm |= host::memory::flags::PERM_WRITE;

    if (flags & PF_X)
        perm |= host::memory::flags::PERM_EXEC;

    return perm;
}

constexpr Word unwrapPermissionFlags(usize perm) {
    Word flags;

    if (perm & host::memory::flags::PERM_READ)
        flags |= PF_R;

    if (perm & host::memory::flags::PERM_WRITE)
        flags |= PF_W;

    if (perm & host::memory::flags::PERM_EXEC)
        flags |= PF_X;

    return flags;
}

} // namespace dashle::binary::elf

#endif /* _DASHLE_BINARY_ELF_H */