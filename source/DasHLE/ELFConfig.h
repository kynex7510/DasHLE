#ifndef _DASHLE_ELFCONFIG_H
#define _DASHLE_ELFCONFIG_H

/* This file shall not be included manually, use the specifications instead. */
#if defined(ELF_HAS_CONFIG)

namespace dashle::elf {

inline const auto& getHeader = &utils::elf::getHeader<Config>;
inline const auto& getSegments = &utils::elf::getSegments<Config>;
inline const auto& getSegmentAllocSize = &utils::elf::getSegmentAllocSize<Config>;
inline const auto& getDynEntries = &utils::elf::getDynEntries<Config>;
inline const auto& getDynEntry = &utils::elf::getDynEntry<Config>;
inline const auto& getSymTab = &utils::elf::getSymTab<Config>;
inline const auto& getSymbolName = &utils::elf::getSymbolName<Config>;

} // namespace dashle::emu::arm::elf

#endif // ELF_HAS_CONFIG

#endif /* _DASHLE_ELFCONFIG_H */