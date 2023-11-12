#ifndef _DASHLE_EMU_ARM_ELF_H
#define _DASHLE_EMU_ARM_ELF_H

#include "DasHLE/Internal/IELF.h"

namespace dashle::elf {
    
struct Config : internal::ielf::Config32, internal::ielf::ConfigLE {
    constexpr static auto ARCH = internal::ielf::constants::EM_ARM;
};

} // namespace dashle::elf

#include "DasHLE/Internal/MakeELF.h"

#endif /* _DASHLE_EMU_ARM_ELF_H */