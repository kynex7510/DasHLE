#ifndef _DASHLE_GUEST_ARM_CONTEXT_H
#define _DASHLE_GUEST_ARM_CONTEXT_H

#include "dynarmic/interface/exclusive_monitor.h"

#include "DasHLE/Guest/Context.h"
#include "DasHLE/Guest/ARM/Environment.h"

#include <unordered_map>

namespace dashle::guest::arm {

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

    /*
    template <typename Self>
    DeducedConst<Self, ARMEnvironment*> environment(this Self&& self) {
        return self.m_Env.get();
    }
    */

    ARMEnvironment* environment() { return m_Env.get(); }

    uaddr invalidAddr() /*const*/ { return environment()->memoryManager()->invalidAddr(); }

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