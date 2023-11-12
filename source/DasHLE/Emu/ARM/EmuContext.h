#ifndef _DASHLE_EMU_ARM_EMUCONTEXT_H
#define _DASHLE_EMU_ARM_EMUCONTEXT_H

#include "DasHLE/Emu/ARM/Environment.h"

namespace dashle::emu::arm {

constexpr static usize REG_R0 = 0;
constexpr static usize REG_R1 = 1;
constexpr static usize REG_R2 = 2;
constexpr static usize REG_R3 = 3;
constexpr static usize REG_R4 = 4;
constexpr static usize REG_R5 = 6;
constexpr static usize REG_R6 = 6;
constexpr static usize REG_R7 = 7;
constexpr static usize REG_R8 = 8;
constexpr static usize REG_R9 = 9;
constexpr static usize REG_R10 = 10;
constexpr static usize REG_R11 = 11;
constexpr static usize REG_R12 = 12;
constexpr static usize REG_R13 = 13;
constexpr static usize REG_R14 = 14;
constexpr static usize REG_R15 = 15;

constexpr static usize REG_SP = REG_R13;
constexpr static usize REG_LR = REG_R14;
constexpr static usize REG_PC = REG_R15;

class EmuContext final {
    std::unique_ptr<Environment> m_Env;
    std::unique_ptr<dynarmic32::Jit> m_Jit;

    dynarmic32::UserConfig buildConfig() const;

public:
    EmuContext();

    const Environment* env() { return m_Env.get(); }

    Expected<void> openBinary(const host::fs::path& path);

    void initCpu();
    void setRegister(usize index, usize value);
    usize getRegister(usize index) const;
    void setThumb(bool thumb);
    void execute(uaddr addr);
};

} // dashle::emu::arm

#endif /* _DASHLE_ARM_EMUCONTEXT_H */