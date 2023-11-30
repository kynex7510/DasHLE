#ifndef _DASHLE_GUEST_ARM_CONTEXT_H
#define _DASHLE_GUEST_ARM_CONTEXT_H

#include "dynarmic/interface/exclusive_monitor.h"
#include "dynarmic/frontend/A32/a32_types.h"

#include "DasHLE/Guest/Context.h"
#include "DasHLE/Guest/ARM/Environment.h"

namespace dashle::guest::arm {

namespace regs {

constexpr static auto R0 = static_cast<usize>(dynarmic32::Reg::R0);
constexpr static auto R1 = static_cast<usize>(dynarmic32::Reg::R1);
constexpr static auto R2 = static_cast<usize>(dynarmic32::Reg::R2);
constexpr static auto R3 = static_cast<usize>(dynarmic32::Reg::R3);
constexpr static auto R4 = static_cast<usize>(dynarmic32::Reg::R4);
constexpr static auto R5 = static_cast<usize>(dynarmic32::Reg::R5);
constexpr static auto R6 = static_cast<usize>(dynarmic32::Reg::R6);
constexpr static auto R7 = static_cast<usize>(dynarmic32::Reg::R7);
constexpr static auto R8 = static_cast<usize>(dynarmic32::Reg::R8);
constexpr static auto R9 = static_cast<usize>(dynarmic32::Reg::R9);
constexpr static auto R10 = static_cast<usize>(dynarmic32::Reg::R10);
constexpr static auto R11 = static_cast<usize>(dynarmic32::Reg::R11);
constexpr static auto R12 = static_cast<usize>(dynarmic32::Reg::R12);
constexpr static auto R13 = static_cast<usize>(dynarmic32::Reg::R13);
constexpr static auto R14 = static_cast<usize>(dynarmic32::Reg::R14);
constexpr static auto R15 = static_cast<usize>(dynarmic32::Reg::R15);
constexpr static auto CPSR = R15 + 1;
constexpr static auto FPSCR = R15 + 2;

constexpr static auto SP = R13;
constexpr static auto LR = R14;
constexpr static auto PC = R15;

} // namespace dashle::guest::arm::regs

class ARMContext final : public Context {
    std::unique_ptr<ARMEnvironment> m_Env;
    std::unique_ptr<dynarmic::ExclusiveMonitor> m_ExMon;
    std::unique_ptr<dynarmic32::Jit> m_Jit;
    std::vector<uaddr> m_Initializers;
    std::vector<uaddr> m_Finalizers;

    void initJit();
    void setPC(uaddr addr);

public:
    ARMContext();

    Expected<void> loadBinary(const std::span<const u8> buffer) override;
    void reset() override;
    dynarmic::HaltReason execute(uaddr addr) override;
    dynarmic::HaltReason step(uaddr addr) override;
    void setRegister(usize id, u64 value) override;
    u64 getRegister(usize id) const override;
    void dump() const override;

    template <typename Self>
    DeducedConst<Self, ARMEnvironment*> environment(this Self&& self) {
        return self.m_Env.get();
    }

    uaddr invalidAddr() const { return environment()->memoryManager()->invalidAddr(); }

    dynarmic::HaltReason execute() override {
        return execute(invalidAddr());
    }

    dynarmic::HaltReason step() override {
        return step(invalidAddr());
    }

    void clearCache() override {
        DASHLE_ASSERT(m_Jit);
        m_Jit->ClearCache();
    }

    void invalidateCache(uaddr addr, usize size) override {
        DASHLE_ASSERT(m_Jit);
        m_Jit->InvalidateCacheRange(addr, size);
    }
};

} // namespace dashle::guest::arm

#endif /* _DASHLE_GUEST_ARM_CONTEXT_H */