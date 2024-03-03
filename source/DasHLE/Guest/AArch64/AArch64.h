#ifndef _DASHLE_GUEST_ARCH_AARCH64_H
#define _DASHLE_GUEST_ARCH_AARCH64_H

#if defined(DASHLE_HAS_GUEST_AARCH64)

#include "DasHLE/Host/Memory.h"
#include "DasHLE/Host/Bridge.h"
#include "DasHLE/Guest/StackVM.h"

#include <unordered_map>

namespace dashle::guest::aarch64 {

class AArch64VM final : public StackVM {
    class Environment;

    std::unique_ptr<Environment> m_Env;
    std::unique_ptr<dynarmic::ExclusiveMonitor> m_ExMon;
    std::unique_ptr<dynarmic64::Jit> m_Jit;

public:
    AArch64VM(std::shared_ptr<host::memory::MemoryManager> mem, std::shared_ptr<host::bridge::Bridge> bridge);

    void setupJit();
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
};

} // namespace dashle::guest::aarch64

#endif // DASHLE_HAS_GUEST_AARCH64

#endif /* _DASHLE_GUEST_ARCH_AARCH64_H */