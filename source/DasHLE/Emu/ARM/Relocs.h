#ifndef _DASHLE_EMU_ARM_RELOCS_H
#define _DASHLE_EMU_ARM_RELOCS_H

#include "DasHLE/Emu/ARM/ELF.h"

namespace dashle::emu::arm {

class RelocationDelegate : public host::memory::TranslatorDelegate {
public:
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