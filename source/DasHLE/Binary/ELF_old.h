#ifndef _DASHLE_BINARY_ELF_H
#define _DASHLE_BINARY_ELF_H

#include "DasHLE/Host/Memory.h"
//#include "DasHLE/Binary/ELFDefs.h"

#include <vector>

namespace dashle::binary {

/*

template <elf::ConfigType CFG>
class Binary final {
    const std::vector<u8> m_Buffer;
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



public:
    Binary(std::vector<u8>&& buffer) : m_Buffer(buffer) {
        DASHLE_ASSERT_WRAPPER_CONST(header, elf::getHeader<CFG>(m_Buffer));
        m_Header = header;
        DASHLE_ASSERT(visitRelocs());
    }

    const std::vector<RelocInfo>& relocs() { return m_Relocs; }
};
*/

} // dashle::binary

#endif /* _DASHLE_ELF_H */