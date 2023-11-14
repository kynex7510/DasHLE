#ifndef _DASHLE_INTERNAL_MAKEELF_H
#define _DASHLE_INTERNAL_MAKEELF_H

#include "DasHLE/Host/Memory.h"

/* This file shall not be included manually, use the specializations instead. */
IELF_ASSERT_CONFIG(dashle::elf::Config);

namespace dashle::elf {

using namespace binary::ielf::constants;

inline const auto& getHeader = &binary::ielf::getHeader<Config>;
inline const auto& getSegments = &binary::ielf::getSegments<Config>;
inline const auto& getSegmentAllocBase = &binary::ielf::getSegmentAllocBase<Config>;
inline const auto& getSegmentAllocSize = &binary::ielf::getSegmentAllocSize<Config>;
inline const auto& getDynEntries = &binary::ielf::getDynEntries<Config>;
inline const auto& getDynEntry = &binary::ielf::getDynEntry<Config>;
inline const auto& getSymTab = &binary::ielf::getSymTab<Config>;
inline const auto& getSymbolName = &binary::ielf::getSymbolName<Config>;
inline const auto& getInitArrayInfo = &binary::ielf::getInitArrayInfo<Config>;
inline const auto& getFiniArrayInfo = &binary::ielf::getFiniArrayInfo<Config>;

constexpr static usize wrapPermissionFlags(Config::WordType flags) {
    usize perm = 0;

    if (flags & PF_R)
        perm |= host::memory::flags::READ;

    if (flags & PF_W)
        perm |= host::memory::flags::WRITE;

    if (flags & PF_X)
        perm |= host::memory::flags::EXEC;

    return perm;
}

} // namespace dashle::elf

#endif /* _DASHLE_INTERNAL_MAKEELF_H */