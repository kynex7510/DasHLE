#ifndef _DASHLE_GUEST_ARM_H
#define _DASHLE_GUEST_ARM_H

#include "DasHLE/Binary/Binary.h"
#include "DasHLE/Host/Interop.h"
#include "DasHLE/Guest/VM.h"

#include <unordered_map>

namespace dashle::guest::arm {

constexpr static usize PAGE_SIZE = 0x1000; // 4KB
static_assert(dashle::isPowerOfTwo(PAGE_SIZE));

constexpr static usize STACK_SIZE = 1024 * 1024; // 1MB
static_assert(dashle::align<usize>(STACK_SIZE, PAGE_SIZE) == STACK_SIZE);

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

class VMImpl final : public GuestVM {
    class Environment;

    const host::interop::SymResolver* m_SymResolver;
    std::unique_ptr<Environment> m_Env;
    std::unique_ptr<dynarmic::ExclusiveMonitor> m_ExMon;
    std::unique_ptr<dynarmic32::Jit> m_Jit;
    uaddr m_StackBase = 0;
    uaddr m_StackTop = 0;
    std::vector<uaddr> m_Initializers;
    std::vector<uaddr> m_Finalizers;

    void setupJit(binary::Version version);
    void setPC(uaddr addr);

public:
    VMImpl(std::shared_ptr<host::memory::MemoryManager> mem, std::shared_ptr<host::interop::InteropHandler> interop,
        const host::interop::SymResolver* resolver);

    ~VMImpl();

    Expected<void> loadBinary(const std::span<const u8> buffer) override;

    Expected<uaddr> virtualToHost(uaddr vaddr) const override;

    dynarmic::HaltReason execute(Optional<uaddr> addr = {}) override;
    dynarmic::HaltReason step(Optional<uaddr> addr = {}) override;
    void runInitializers() override;

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

#endif /* _DASHLE_GUEST_ARM_H */