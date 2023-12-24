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

        return dashle::align(segmentVaddr, segmentAlign);
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
        return dashle::alignOver(segmentMemsz, segmentAlign);

    return segmentMemsz;
}

Expected<void> ELF::parse(std::vector<u8>&& buffer) {
    m_Buffer = std::move(buffer);

    // Detect bitness.
    if (m_Buffer.size() < sizeof(Ehdr32))
        return Unexpected(Error::InvalidSize);

    const auto bits64 = reinterpret_cast<const Ehdr32*>(binaryBase())->e_ident[EI_CLASS] == ELFCLASS64;

    // Set header.
    if (bits64) {
        m_Header.initialize<Header32>(reinterpret_cast<const Ehdr32*>(binaryBase()));
    } else {
        m_Header.initialize<Header64>(reinterpret_cast<const Ehdr64*>(binaryBase()));
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

    // Get section headers.
    if (bits64) {
        const auto entries = reinterpret_cast<const Shdr64*>(binaryBase() + m_Header->shoff());
        for (auto i = 0u; i < m_Header->shnum(); ++i)
            m_SectionHeaders.emplace_back().initialize<SectionHeader64>(&entries[i]);
    } else {
        const auto entries = reinterpret_cast<const Shdr32*>(binaryBase() + m_Header->shoff());
        for (auto i = 0u; i < m_Header->shnum(); ++i)
            m_SectionHeaders.emplace_back().initialize<SectionHeader32>(&entries[i]);
    }

    // Get program headers.
     if (bits64) {
        const auto entries = reinterpret_cast<const Phdr64*>(binaryBase() + m_Header->phoff());
        for (auto i = 0u; i < m_Header->phnum(); ++i)
            m_ProgramHeaders.emplace_back().initialize<ProgramHeader64>(&entries[i]);
    } else {
        const auto entries = reinterpret_cast<const Phdr32*>(binaryBase() + m_Header->phoff());
        for (auto i = 0u; i < m_Header->phnum(); ++i)
            m_ProgramHeaders.emplace_back().initialize<ProgramHeader32>(&entries[i]);
    }


    // Detect binary version.
    // TODO
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
                    entries.emplace_back().initialize<DynEntry64>(entry);

                ++entry;
            }
        } else {
            auto entry = reinterpret_cast<const Dyn32*>(binaryBase() + dyn->offset());
            while (entry->d_tag != DT_NULL) {
                if (entry->d_tag == tag)
                    entries.emplace_back().initialize<DynEntry32>(entry);

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