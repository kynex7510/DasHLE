#ifndef _DASHLE_EMU_ARM_ELF_H
#define _DASHLE_EMU_ARM_ELF_H

#include "DasHLE/Binary/IELF.h"

namespace dashle::elf {
    
struct Config : binary::ielf::Config32, binary::ielf::ConfigLE {
    constexpr static auto ARCH = binary::ielf::constants::EM_ARM;
};

} // namespace dashle::elf

#include "DasHLE/Binary/MakeELF.h"

#endif /* _DASHLE_EMU_ARM_ELF_H */