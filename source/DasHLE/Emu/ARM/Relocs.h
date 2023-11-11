#ifndef _DASHLE_EMU_ARM_RELOCS_H
#define _DASHLE_EMU_ARM_RELOCS_H

#include "DasHLE/Emu/ARM/ELFConfig.h"

namespace dashle::emu::arm {

struct RelocationDelegate {
    virtual Expected<uaddr> virtualToHost(uaddr vaddr) const = 0;
    virtual Expected<uaddr> resolveSymbol(const std::string& symbolName) = 0;
};

struct RelocationContext {
    const elf::Config::HeaderType* header;
    uaddr virtualBase;
    RelocationDelegate* resolver;
};

Expected<void> handleRelocations(const RelocationContext& ctx);

} // namespace dashle::emu::arm

#endif /* _DASHLE_EMU_ARM_RELOCS_H */