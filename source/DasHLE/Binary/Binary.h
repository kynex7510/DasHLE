#ifndef _DASHLE_BINARY_BINARY_H
#define _DASHLE_BINARY_BINARY_H

#include "DasHLE/Host/Memory.h"
#include "DasHLE/Binary/ELF.h"

namespace dashle::binary {

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
    soff patchOffset;
    soff addend;
    RelocKind kind;
    Optional<std::string> symbolName;
};

template <elf::ConfigType CFG>
constexpr usize wrapPermissionFlags(typename CFG::WordType flags) {
    usize perm = 0;
        
    if (flags & elf::PF_R)
        perm |= host::memory::flags::PERM_READ;

    if (flags & elf::PF_W)
        perm |= host::memory::flags::PERM_WRITE;

    if (flags & elf::PF_X)
        perm |= host::memory::flags::PERM_EXEC;

    return perm;
}

template <elf::ConfigType CFG>
constexpr CFG::WordType unwrapPermissionFlags(usize perm) {
    typename CFG::WordType flags;

    if (perm & host::memory::flags::PERM_READ)
        flags |= elf::PF_R;

    if (perm & host::memory::flags::PERM_WRITE)
        flags |= elf::PF_W;

    if (perm & host::memory::flags::PERM_EXEC)
        flags |= elf::PF_X;

    return flags;
}

template <elf::ConfigType CFG>
class Binary final {
    const std::span<const u8> m_Buffer;
    const typename CFG::HeaderType* m_Header;
    std::vector<RelocInfo> m_Relocs;

    constexpr static bool isRelativeReloc(usize type) {
        return type == elf::constants::R_ARM_RELATIVE || type == elf::constants::R_AARCH64_RELATIVE;
    }

    constexpr static bool isSymbolReloc(usize type) {
        return type == elf::constants::R_ARM_ABS32 ||
            type == elf::constants::R_ARM_GLOB_DAT ||
            type == elf::constants::R_ARM_JUMP_SLOT ||
            type == elf::constants::R_AARCH64_ABS64 ||
            type == elf::constants::R_AARCH64_GLOB_DAT ||
            type == elf::constants::R_AARCH64_JUMP_SLOT;
    }

    auto base() const { return reinterpret_cast<uaddr>(m_Buffer.data()); }
    auto header() const { return m_Header; }

    Expected<void> visitRelArray(const typename CFG::RelType* relArray, usize size) {
        for (auto i = 0u; i < size; ++i) {
            const auto rel = &relArray[i];

            if (isRelativeReloc(rel->type())) {
                m_Relocs.push_back(RelocInfo {
                    .patchOffset = rel->r_offset,
                    .addend = 0,
                    .kind = RelocKind::Relative,
                });
                continue;
            }

            if (isSymbolReloc(rel->type())) {
                DASHLE_TRY_EXPECTED_CONST(symbolName, elf::getSymbolName<CFG>(header(), rel->symbolIndex()));
                m_Relocs.push_back(RelocInfo {
                    .patchOffset = rel->r_offset,
                    .addend = 0,
                    .kind = RelocKind::Symbol,
                    .symbolName = symbolName,
                });
                continue;
            }

            return Unexpected(Error::InvalidRelocation);
        }

        return EXPECTED_VOID;
    }
    
    Expected<void> visitRelaArray(const typename CFG::RelaType* relaArray, usize size) {
        for (auto i = 0u; i < size; ++i) {
            const auto rela = &relaArray[i];

            if (isRelativeReloc(rela->type())) {
                m_Relocs.push_back(RelocInfo {
                    .patchOffset = rela->r_offset,
                    .addend = rela->r_addend,
                    .kind = RelocKind::Relative,
                });
                continue;
            }

            if (isSymbolReloc(rela->type())) {
                DASHLE_TRY_EXPECTED_CONST(symbolName, elf::getSymbolName<CFG>(header(), rela->symbolIndex()));
                m_Relocs.push_back(RelocInfo {
                    .patchOffset = rela->r_offset,
                    .addend = rela->r_addend,
                    .kind = RelocKind::Symbol,
                    .symbolName = symbolName,
                });
                continue;
            }

            return Unexpected(Error::InvalidRelocation);
        }

        return EXPECTED_VOID;
    }

    Expected<void> visitRel() {
        const auto relEntry = dynEntry(elf::DT_REL);
        const auto relEntrySize = dynEntry(elf::DT_RELSZ);
        const auto relEntryEnt = dynEntry(elf::DT_RELENT);

        if (relEntry && relEntrySize && relEntryEnt) {
            const auto relArray = reinterpret_cast<CFG::RelType*>(base() + relEntry.value()->d_un.d_ptr);
            const usize size = relEntrySize.value()->d_un.d_val / relEntryEnt.value()->d_un.d_val;
            return visitRelArray(relArray, size);
        }

        return EXPECTED_VOID;
    }

    Expected<void> visitRela() {
        const auto relaEntry = dynEntry(elf::DT_RELA);
        const auto relaEntrySize = dynEntry(elf::DT_RELASZ);
        const auto relaEntryEnt = dynEntry(elf::DT_RELAENT);

        if (relaEntry && relaEntrySize && relaEntryEnt) {
            const auto relaArray = reinterpret_cast<CFG::RelaType*>(base() + relaEntry.value()->d_un.d_ptr);
            const usize size = relaEntrySize.value()->d_un.d_val / relaEntryEnt.value()->d_un.d_val;
            return visitRelaArray(relaArray, size);
        }

        return EXPECTED_VOID;
    }

    Expected<void> visitJmprel() {
        const auto jmprelEntry = dynEntry(elf::DT_JMPREL);
        const auto jmprelEntrySize = dynEntry(elf::DT_PLTRELSZ);
        const auto jmprelEntryType = dynEntry(elf::DT_PLTREL);

        if (!jmprelEntry || !jmprelEntrySize || !jmprelEntryType)
            return EXPECTED_VOID;

        if (jmprelEntryType.value()->d_un.d_val == elf::DT_REL) {
            const auto jmprelArray = reinterpret_cast<CFG::RelType*>(base() + jmprelEntry.value()->d_un.d_ptr);
            const usize size = jmprelEntrySize.value()->d_un.d_val / sizeof(typename CFG::RelType);   
            return visitRelArray(jmprelArray, size);
        }

        if (jmprelEntryType.value()->d_un.d_val == elf::DT_RELA) {
            const auto jmprelArray = reinterpret_cast<CFG::RelaType*>(base() + jmprelEntry.value()->d_un.d_ptr);
            const usize size = jmprelEntrySize.value()->d_un.d_val / sizeof(typename CFG::RelaType);
            return visitRelaArray(jmprelArray, size);
        }

        return Unexpected(Error::InvalidRelocation);
    }

    Expected<void> visitRelocations() {
        return visitRel().and_then([this] {
            return visitRela();
        }).and_then([this] {
            return visitJmprel();
        });
    }

public:
    Binary(const std::span<const u8> buffer) : m_Buffer(buffer) {
        DASHLE_ASSERT_WRAPPER_CONST(header, elf::getHeader<CFG>(m_Buffer));
        m_Header = header;
        DASHLE_ASSERT(visitRelocations());
    }

    const std::vector<RelocInfo>& relocs() { return m_Relocs; }

    static inline const auto& wrapPermissionFlags = binary::wrapPermissionFlags<CFG>;
    static inline const auto& unwrapPermissionFlags = binary::unwrapPermissionFlags<CFG>;
    static inline const auto& segmentAllocOffset = &elf::getSegmentAllocOffset<CFG>;
    static inline const auto& segmentAllocSize = &elf::getSegmentAllocSize<CFG>;

    auto segments(typename CFG::WordType type) const { return elf::getSegments<CFG>(header(), type); }
    auto dynEntries(typename CFG::SwordType tag) const { return elf::getDynEntries<CFG>(header(), tag); }
    auto dynEntry(typename CFG::SwordType tag) const { return elf::getDynEntry<CFG>(header(), tag); }
    auto symTab() const { return elf::getSymTab<CFG>(header()); }
    auto symbolName(typename CFG::WordType index) const { return elf::getSymbolName<CFG>(header(), index); }
    auto initArrayInfo() const { return elf::getInitArrayInfo<CFG>(header()); }
    auto finiArrayInfo() const { return elf::getFiniArrayInfo<CFG>(header()); }
};

} // dashle::binary

#endif /* _DASHLE_BINARY_BINARY_H */