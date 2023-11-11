#ifndef _DASHLE_EMU_ARM_ELFCONFIG_H
#define _DASHLE_EMU_ARM_ELFCONFIG_H

#include "DasHLE/Utils/ELF.h"

#define ELF_HAS_CONFIG

namespace dashle::emu::arm {

using namespace utils::elf::constants;

struct Config : utils::elf::Config32, utils::elf::ConfigLE {
    constexpr static auto ARCH = EM_ARM;
};

ELF_ASSERT_CONFIG(Config);

}; // namespace dashle::emu::arm

namespace dashle::elf {
    using Config = dashle::emu::arm::Config;
} // namespace dashle::elf

#include "DasHLE/ELFConfig.h"

#endif /* _DASHLE_EMU_ARM_ELFCONFIG_H */