#include "DasHLE/Support/Math.h"
#include "DasHLE/Binary/ELF.h"

using namespace dashle;
using namespace dashle::binary::elf;

// ELF

Expected<uaddr> IProgramHeader::allocationOffset() const {
    const auto segmentAlign = align();
    const auto segmentVaddr = vaddr();
    const auto segmentOffset = offset();

    if (segmentAlign > 1) {
        if ((segmentVaddr % segmentAlign) != (segmentOffset % segmentAlign))
            return Unexpected(Error::InvalidSegment);

        return dashle::alignDown(segmentVaddr, segmentAlign);
    }

    return segmentVaddr;
}

Expected<usize> IProgramHeader::allocationSize() const {
    const auto segmentMemsz = memsz();
    const auto segmentFilesz = filesz();
    const auto segmentAlign = align();

   if (segmentMemsz < segmentFilesz)
    return Unexpected(Error::InvalidSegment);

    if (segmentAlign > 1)
        return dashle::alignUp(segmentMemsz, segmentAlign);

    return segmentMemsz;
}

constexpr static bool isRelativeReloc(usize type) {
    return type == R_ARM_RELATIVE || type == R_AARCH64_RELATIVE;
}

constexpr static bool isSymbolReloc(usize type) {
    return type == R_ARM_ABS32 || type == R_ARM_GLOB_DAT || type == R_ARM_JUMP_SLOT ||
        type == R_AARCH64_ABS64 || type == R_AARCH64_GLOB_DAT || type == R_AARCH64_JUMP_SLOT;
}

Expected<void> ELF::visitRelArray32(const Rel32* relArray, usize size) {
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
            DASHLE_TRY_EXPECTED_CONST(symbol, symbolByIndex(rel->symbolIndex()));
            DASHLE_TRY_EXPECTED_CONST(symbolName, stringByOffset(symbol->name()));
            m_Relocs.push_back(RelocInfo {
                .patchOffset = rel->r_offset,
                .addend = 0,
                .kind = RelocKind::Symbol,
                .symbol = symbolName,
            });
            continue;
        }

        return Unexpected(Error::InvalidRelocation);
    }

    return EXPECTED_VOID;
}

Expected<void> ELF::visitRelaArray32(const Rela32* relaArray, usize size) {
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
            DASHLE_TRY_EXPECTED_CONST(symbol, symbolByIndex(rela->symbolIndex()));
            DASHLE_TRY_EXPECTED_CONST(symbolName, stringByOffset(symbol->name()));
            m_Relocs.push_back(RelocInfo {
                .patchOffset = rela->r_offset,
                .addend = rela->r_addend,
                .kind = RelocKind::Symbol,
                .symbol = symbolName,
            });
            continue;
        }

        return Unexpected(Error::InvalidRelocation);
    }

    return EXPECTED_VOID;
}

Expected<void> ELF::visitRel() {
    const auto relEntryWrapper = dynEntryWithTag(DT_REL);
    const auto relEntrySizeWrapper = dynEntryWithTag(DT_RELSZ);
    const auto relEntryEntWrapper = dynEntryWithTag(DT_RELENT);

    if (!relEntryWrapper || !relEntrySizeWrapper || !relEntryEntWrapper)
        return EXPECTED_VOID;

    DASHLE_ASSERT_WRAPPER_CONST(relEntry, relEntryWrapper);
    DASHLE_ASSERT_WRAPPER_CONST(relEntrySize, relEntrySizeWrapper);
    DASHLE_ASSERT_WRAPPER_CONST(relEntryEnt, relEntryEntWrapper);

    const usize size = relEntrySize->val() / relEntryEnt->val();

    if (is64Bits()) {
        const auto relArray = reinterpret_cast<const Rel64*>(binaryBase() + relEntry->ptr());
        return visitRelArray64(relArray, size);
    }

    const auto relArray = reinterpret_cast<const Rel32*>(binaryBase() + relEntry->ptr());
    return visitRelArray32(relArray, size);
}

Expected<void> ELF::visitRela() {
    const auto relaEntryWrapper = dynEntryWithTag(DT_RELA);
    const auto relaEntrySizeWrapper = dynEntryWithTag(DT_RELASZ);
    const auto relaEntryEntWrapper = dynEntryWithTag(DT_RELAENT);

    if (!relaEntryWrapper || !relaEntrySizeWrapper || !relaEntryEntWrapper)
        return EXPECTED_VOID;

    DASHLE_ASSERT_WRAPPER_CONST(relaEntry, relaEntryWrapper);
    DASHLE_ASSERT_WRAPPER_CONST(relaEntrySize, relaEntrySizeWrapper);
    DASHLE_ASSERT_WRAPPER_CONST(relaEntryEnt, relaEntryEntWrapper);

    const usize size = relaEntrySize->val() / relaEntryEnt->val();

    if (is64Bits()) {
        const auto relaArray = reinterpret_cast<const Rela64*>(binaryBase() + relaEntry->ptr());
        return visitRelaArray64(relaArray, size);
    }

    const auto relaArray = reinterpret_cast<const Rela32*>(binaryBase() + relaEntry->ptr());
    return visitRelaArray32(relaArray, size);
}

Expected<void> ELF::visitJmprel() {
    const auto jmprelEntryWrapper = dynEntryWithTag(DT_JMPREL);
    const auto jmprelEntrySizeWrapper = dynEntryWithTag(DT_PLTRELSZ);
    const auto jmprelEntryTypeWrapper = dynEntryWithTag(DT_PLTREL);

    if (!jmprelEntryWrapper || !jmprelEntrySizeWrapper || !jmprelEntryTypeWrapper)
        return EXPECTED_VOID;

    DASHLE_ASSERT_WRAPPER_CONST(jmprelEntry, jmprelEntryWrapper);
    DASHLE_ASSERT_WRAPPER_CONST(jmprelEntrySize, jmprelEntrySizeWrapper);
    DASHLE_ASSERT_WRAPPER_CONST(jmprelEntryType, jmprelEntryTypeWrapper);

    if (jmprelEntryType->val() == DT_REL) {
        if (is64Bits()) {
            const auto relArray = reinterpret_cast<const Rel64*>(binaryBase() + jmprelEntry->ptr());
            const usize size = jmprelEntrySize->val() / sizeof(Rel64);
            return visitRelArray64(relArray, size);
        }
        
        const auto relArray = reinterpret_cast<const Rel32*>(binaryBase() + jmprelEntry->ptr());
        const usize size = jmprelEntrySize->val() / sizeof(Rel32);
        return visitRelArray32(relArray, size);
    }

    if (jmprelEntryType->val() == DT_RELA) {
        if (is64Bits()) {
            const auto relaArray = reinterpret_cast<const Rela64*>(binaryBase() + jmprelEntry->ptr());
            const usize size = jmprelEntrySize->val() / sizeof(Rela64);
            return visitRelaArray64(relaArray, size);
        }

        const auto relaArray = reinterpret_cast<const Rela32*>(binaryBase() + jmprelEntry->ptr());
        const usize size = jmprelEntrySize->val() / sizeof(Rela32);
        return visitRelaArray32(relaArray, size);
    }

    return Unexpected(Error::InvalidRelocation);
}

Expected<void> ELF::visitRelocs() {
    return visitRel().and_then([this] {
        return visitRela();
    }).and_then([this] {
        return visitJmprel();
    });
}

Expected<void> ELF::parse(std::vector<u8>&& buffer) {
    m_Buffer = std::move(buffer);

    // Detect bitness.
    if (m_Buffer.size() < sizeof(Ehdr32))
        return Unexpected(Error::InvalidSize);

    const auto bits64 = reinterpret_cast<const Ehdr32*>(binaryBase())->e_ident[EI_CLASS] == ELFCLASS64;

    // Set header.
    if (bits64) {
        m_Header = Header32(reinterpret_cast<const Ehdr32*>(binaryBase()));
    } else {
        m_Header = Header64(reinterpret_cast<const Ehdr64*>(binaryBase()));
    }

    // Check size.
    if ((bits64 && buffer.size() < sizeof(Ehdr64)) ||
        (!bits64 && buffer.size() < sizeof(Ehdr32)) ||
        m_Buffer.size() < m_Header->ehsize())
        return Unexpected(Error::InvalidSize);

    // Check magic.
    if (!std::equal(m_Header->ident(), m_Header->ident() + SELFMAG, ELFMAG))
        return Unexpected(Error::InvalidMagic);

    // Check data encoding.
    if (m_Header->ident()[EI_DATA] != ELFDATA2LSB)
        return Unexpected(Error::InvalidDataEncoding);

    // Check position indipendent binary.
    if (m_Header->type() != ET_DYN)
        return Unexpected(Error::NoPIE);

    // Platform specific checks.
    if (bits64) {
        if (m_Header->ident()[EI_CLASS] != ELFCLASS64)
            return Unexpected(Error::InvalidClass);

        if (m_Header->machine() != EM_AARCH64)
            return Unexpected(Error::InvalidArch);
    } else {
        if (m_Header->ident()[EI_CLASS] != ELFCLASS32)
            return Unexpected(Error::InvalidClass);

        if (m_Header->machine() != EM_ARM)
            return Unexpected(Error::InvalidArch);
    }

    // Detect binary version.
    // TODO

    // Get section headers.
    if (bits64) {
        const auto entries = reinterpret_cast<const Shdr64*>(binaryBase() + m_Header->shoff());
        for (auto i = 0u; i < m_Header->shnum(); ++i)
            m_SectionHeaders.emplace_back(SectionHeader64(&entries[i]));
    } else {
        const auto entries = reinterpret_cast<const Shdr32*>(binaryBase() + m_Header->shoff());
        for (auto i = 0u; i < m_Header->shnum(); ++i)
            m_SectionHeaders.emplace_back(SectionHeader32(&entries[i]));
    }

    // Get program headers.
     if (bits64) {
        const auto entries = reinterpret_cast<const Phdr64*>(binaryBase() + m_Header->phoff());
        for (auto i = 0u; i < m_Header->phnum(); ++i)
            m_ProgramHeaders.emplace_back(ProgramHeader64(&entries[i]));
    } else {
        const auto entries = reinterpret_cast<const Phdr32*>(binaryBase() + m_Header->phoff());
        for (auto i = 0u; i < m_Header->phnum(); ++i)
            m_ProgramHeaders.emplace_back(ProgramHeader32(&entries[i]));
    }

    // Get string table.
    DASHLE_TRY_EXPECTED_VOID(
        dynEntryWithTag(DT_STRTAB).and_then([this](const DynEntry& strTab) {
            this->m_StrTab = Poly(strTab);
            return EXPECTED_VOID;
        }
    ));

    // Get symbol table.
    DASHLE_TRY_EXPECTED_VOID(
        dynEntryWithTag(DT_SYMTAB).and_then([this](const DynEntry& symTab) {
            this->m_SymTab = Poly(symTab);
            return EXPECTED_VOID;
        }
    ));

    // Get relocations.
    return visitRelocs();
}

Expected<ELF::SectionHeader> ELF::sectionHeader(usize index) const {
    if (index >= m_SectionHeaders.size())
        return Unexpected(Error::InvalidIndex);

    return m_SectionHeaders[index];
}

Expected<ELF::ProgramHeader> ELF::programHeader(usize index) const {
    if (index >= m_ProgramHeaders.size())
        return Unexpected(Error::InvalidIndex);

    return m_ProgramHeaders[index];
}

Expected<std::vector<ELF::SectionHeader>> ELF::sectionsOfType(Word type) const {
    std::vector<SectionHeader> sections;

    for (const auto& section : sections) {
        if (section->type() == type)
            sections.push_back(section);
    }

    return sections;
}

Expected<std::vector<ELF::ProgramHeader>> ELF::segmentsOfType(Word type) const {
    std::vector<ProgramHeader> segments;

    for (const auto& segment : segments) {
        if (segment->type() == type)
            segments.push_back(segment);
    }

    return segments;
}

Expected<std::vector<ELF::DynEntry>> ELF::dynEntriesWithTag(Sword tag) const {
    std::vector<DynEntry> entries;
    DASHLE_TRY_EXPECTED_CONST(dynSegments, segmentsOfType(PT_DYNAMIC));

    for (const auto& dyn : dynSegments) {
        if (is64Bits()) {
            auto entry = reinterpret_cast<const Dyn64*>(binaryBase() + dyn->offset());
            while (entry->d_tag != DT_NULL) {
                if (entry->d_tag == tag)
                    entries.emplace_back(DynEntry64(entry));

                ++entry;
            }
        } else {
            auto entry = reinterpret_cast<const Dyn32*>(binaryBase() + dyn->offset());
            while (entry->d_tag != DT_NULL) {
                if (entry->d_tag == tag)
                    entries.emplace_back(DynEntry32(entry));

                ++entry;
            }
        }
    }

    return entries;
}

Expected<ELF::DynEntry> ELF::dynEntryWithTag(Sword tag) const {
    DASHLE_TRY_EXPECTED_CONST(entries, dynEntriesWithTag(tag));
    if (entries.size() != 1)
        return Unexpected(Error::InvalidSize);

    return entries[0];
}

Expected<ELF::SymEntry> ELF::symbolByIndex(usize index) const {
    if (index == STN_UNDEF)
        return Unexpected(Error::InvalidIndex);

    if (!m_SymTab.holdsAny())
        return Unexpected(Error::NotFound);

    SymEntry symbol;
    if (is64Bits()) {
        const auto entries = reinterpret_cast<const Sym64*>(binaryBase() + m_SymTab->ptr());
        symbol = SymEntry64(&entries[index]);
    } else {
        const auto entries = reinterpret_cast<const Sym32*>(binaryBase() + m_SymTab->ptr());
        symbol = SymEntry32(&entries[index]);
    }

    return symbol;
}

Expected<std::string> ELF::stringByOffset(usize offset) const {
    if (!m_StrTab.holdsAny())
        return Unexpected(Error::NotFound);
    return reinterpret_cast<const char*>(binaryBase() + m_StrTab->ptr() + offset);
}

Optional<FuncArrayInfo> ELF::initArrayInfo() const {
    const auto initEntryWrapper = dynEntryWithTag(DT_INIT_ARRAY);
    const auto initEntrySzWrapper = dynEntryWithTag(DT_INIT_ARRAYSZ);

    if (initEntryWrapper && initEntrySzWrapper) {
        DASHLE_ASSERT_WRAPPER_CONST(initEntry, initEntryWrapper);
        DASHLE_ASSERT_WRAPPER_CONST(initEntrySz, initEntrySzWrapper);
        const auto addrSize = is64Bits() ? sizeof(Addr64) : sizeof(Addr32);
        return FuncArrayInfo {
            .offset = initEntry->ptr(),
            .size = initEntrySz->val() / addrSize,
        };
    }

    return {};
}

Optional<FuncArrayInfo> ELF::finiArrayInfo() const {
    const auto finiEntryWrapper = dynEntryWithTag(DT_FINI_ARRAY);
    const auto finiEntrySzWrapper = dynEntryWithTag(DT_FINI_ARRAYSZ);

    if (finiEntryWrapper && finiEntrySzWrapper) {
        DASHLE_ASSERT_WRAPPER_CONST(finiEntry, finiEntryWrapper);
        DASHLE_ASSERT_WRAPPER_CONST(finiEntrySz, finiEntrySzWrapper);
        const auto addrSize = is64Bits() ? sizeof(Addr64) : sizeof(Addr32);
        return FuncArrayInfo {
            .offset = finiEntry->ptr(),
            .size = finiEntrySz->val() / addrSize,
        };
    }

    return {};
}