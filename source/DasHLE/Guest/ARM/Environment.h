#ifndef _DASHLE_GUEST_ARM_ENVIRONMENT_H
#define _DASHLE_GUEST_ARM_ENVIRONMENT_H

#include "dynarmic/interface/A32/a32.h"
#include "DasHLE/Host/Memory.h"

namespace dashle {
namespace dynarmic32 = Dynarmic::A32;
} // namespace dashle

namespace dashle::guest::arm {

constexpr static usize PAGE_SIZE = 0x1000; // 4KB

class ARMEnvironment final : public dynarmic32::UserCallbacks {
    std::unique_ptr<host::memory::MemoryManager> m_Mem;
    uaddr m_StackBase = 0;
    uaddr m_StackTop = 0;

    Expected<uaddr> virtualToHostChecked(uaddr vaddr, usize flags) const;

    /* Dynarmic callbacks */

    bool PreCodeReadHook(bool isThumb, dynarmic32::VAddr pc, dynarmic32::IREmitter& ir) override;
    std::optional<std::uint32_t> MemoryReadCode(dynarmic32::VAddr vaddr) override;

    std::uint8_t MemoryRead8(dynarmic32::VAddr vaddr) override;
    std::uint16_t MemoryRead16(dynarmic32::VAddr vaddr) override;
    std::uint32_t MemoryRead32(dynarmic32::VAddr vaddr) override;
    std::uint64_t MemoryRead64(dynarmic32::VAddr vaddr) override;
    void MemoryWrite8(dynarmic32::VAddr vaddr, std::uint8_t value) override;
    void MemoryWrite16(dynarmic32::VAddr vaddr, std::uint16_t value) override;
    void MemoryWrite32(dynarmic32::VAddr vaddr, std::uint32_t value) override;
    void MemoryWrite64(dynarmic32::VAddr vaddr, std::uint64_t value) override;
    bool MemoryWriteExclusive8(dynarmic32::VAddr vaddr, std::uint8_t value, std::uint8_t expected) override;
    bool MemoryWriteExclusive16(dynarmic32::VAddr vaddr, std::uint16_t value, std::uint16_t expected) override;
    bool MemoryWriteExclusive32(dynarmic32::VAddr vaddr, std::uint32_t value, std::uint32_t expected) override;
    bool MemoryWriteExclusive64(dynarmic32::VAddr vaddr, std::uint64_t value, std::uint64_t expected) override;
    bool IsReadOnlyMemory(dynarmic32::VAddr vaddr) override;

    void InterpreterFallback(dynarmic32::VAddr pc, usize numInstructions) override {
        DASHLE_UNREACHABLE("Interpreter invoked (pc={}, numInstructions={})", pc, numInstructions);
    }

    void CallSVC(u32 swi) override {
        // TODO: support SVC.
        DASHLE_UNREACHABLE("Unimplemented SVC call (swi={})", swi);
    }

    void ExceptionRaised(dynarmic32::VAddr pc, dynarmic32::Exception exception) override {
        // TODO: handle exceptions.
        DASHLE_UNREACHABLE("Unimplemented exception handling (pc=0x{:X}, exception={})", pc, static_cast<u32>(exception));
    }

    void AddTicks(std::uint64_t ticks) override {}
    std::uint64_t GetTicksRemaining() override { return static_cast<u64>(-1); }

public:
    ARMEnvironment(std::unique_ptr<host::memory::MemoryManager> mem, usize stackSize);
    virtual ~ARMEnvironment() {}

    template <typename Self>
    DeducedConst<Self, host::memory::MemoryManager*> memoryManager(this Self&& self) {
        return self.m_Mem.get();
    }

    uaddr stackBase() const { return m_StackBase; }
    uaddr stackTop() const { return m_StackTop; }

    Expected<uaddr> virtualToHost(uaddr vaddr) const;
};

} // namespace dashle::guest::arm

#endif /* _DASHLE_GUEST_ARM_ENVIRONMENT_H */