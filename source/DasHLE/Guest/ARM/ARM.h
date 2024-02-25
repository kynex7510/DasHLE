#ifndef _DASHLE_GUEST_ARM_H
#define _DASHLE_GUEST_ARM_H

#if defined(DASHLE_HAS_GUEST_ARM)

#include "DasHLE/Host/Memory.h"
#include "DasHLE/Host/Bridge.h"
#include "DasHLE/Guest/StackVM.h"

namespace dashle::guest::arm {

namespace regs {

constexpr static usize R0 = 0;
constexpr static usize R1 = 1;
constexpr static usize R2 = 2;
constexpr static usize R3 = 3;
constexpr static usize R4 = 4;
constexpr static usize R5 = 5;
constexpr static usize R6 = 6;
constexpr static usize R7 = 7;
constexpr static usize R8 = 8;
constexpr static usize R9 = 9;
constexpr static usize R10 = 10;
constexpr static usize R11 = 11;
constexpr static usize R12 = 12;
constexpr static usize R13 = 13;
constexpr static usize R14 = 14;
constexpr static usize R15 = 15;

constexpr static usize SP = R13;
constexpr static usize LR = R14;
constexpr static usize PC = R15;
constexpr static usize CPSR = R15 + 1;
constexpr static usize FPSCR = R15 + 2;

} // namespace dashle::guest::arm::regs

class ARMVM final : public StackVM {
    class Environment;

    std::unique_ptr<Environment> m_Env;
    std::unique_ptr<dynarmic::ExclusiveMonitor> m_ExMon;
    std::unique_ptr<dynarmic32::Jit> m_Jit;

    void setPC(uaddr addr);

public:
    ARMVM(std::shared_ptr<host::memory::MemoryManager> mem, std::shared_ptr<host::bridge::Bridge> bridge, GuestVersion version);
    ARMVM(const ARMVM&) = delete;
    ARMVM(ARMVM&&) = default;

    ARMVM& operator=(const ARMVM&) = delete;
    ARMVM& operator=(ARMVM&&) = default;

    dynarmic::HaltReason execute(Optional<uaddr> addr = {}) override;
    dynarmic::HaltReason step(Optional<uaddr> addr = {}) override;

    void clearCache() override {
        DASHLE_ASSERT(m_Jit);
        m_Jit->ClearCache();
    }

    void invalidateCache(uaddr addr, usize size) override {
        DASHLE_ASSERT(m_Jit);
        m_Jit->InvalidateCacheRange(addr, size);
    }

    void setRegister(usize id, u64 value) override;
    u64 getRegister(usize id) const override;

    void dump() const override;
};

} // namespace dashle::guest::arm

#endif // DASHLE_HAS_GUEST_ARM

#endif /* _DASHLE_GUEST_ARM_H */