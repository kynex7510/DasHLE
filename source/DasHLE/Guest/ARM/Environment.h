#ifndef _DASHLE_GUEST_ARM_ENVIRONMENT_H
#define _DASHLE_GUEST_ARM_ENVIRONMENT_H

#include "dynarmic/interface/A32/a32.h"
#include "dynarmic/frontend/A32/a32_types.h"

#include "DasHLE/Host/Memory.h"
#include "DasHLE/Binary/Binary.h"

#include <unordered_map>

namespace dashle {
namespace dynarmic32 = Dynarmic::A32;
} // namespace dashle

namespace dashle::guest::arm {

class ARMContext;

struct ELFConfig : binary::elf::Config32, binary::elf::ConfigLE {
    constexpr static auto ARCH = binary::elf::constants::EM_ARM;
};

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

constexpr static auto SP = R13;
constexpr static auto LR = R14;
constexpr static auto PC = R15;
constexpr static auto CPSR = R15 + 1;
constexpr static auto FPSCR = R15 + 2;

inline dynarmic32::Reg asEnum(usize reg) {
    DASHLE_ASSERT(reg <= R15);
    return static_cast<dynarmic32::Reg>(reg);
}

} // namespace dashle::guest::arm::regs

constexpr static usize PAGE_SIZE = 0x1000; // 4KB

class ARMEnvironment final : public dynarmic32::UserCallbacks {
    std::unique_ptr<host::memory::MemoryManager> m_Mem;
    uaddr m_StackBase = 0;
    uaddr m_StackTop = 0;
    uaddr m_ILTBase = 0;
    usize m_ILTNumEntries = 0;
    std::unordered_map<usize, std::string> m_ILTEntries;

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

    void CallSVC(u32 swi) override;

    void ExceptionRaised(dynarmic32::VAddr pc, dynarmic32::Exception exception) override {
        // TODO: handle exceptions.
        DASHLE_UNREACHABLE("Unimplemented exception handling (pc=0x{:X}, exception={})", pc, static_cast<u32>(exception));
    }

    void AddTicks(std::uint64_t ticks) override {}
    std::uint64_t GetTicksRemaining() override { return static_cast<u64>(-1); }

public:
    ARMContext* m_Ctx = nullptr;

    ARMEnvironment(std::unique_ptr<host::memory::MemoryManager> mem, usize stackSize);
    virtual ~ARMEnvironment() noexcept {}

    // TODO: explicit object parameters aren't widely supported yet *sigh*.

    /*
    template <typename Self>
    DeducedConst<Self, host::memory::MemoryManager*> memoryManager(this Self&& self) {
        return self.m_Mem.get();
    }
    */

    host::memory::MemoryManager* memoryManager() { return m_Mem.get(); }

    uaddr stackBase() const { return m_StackBase; }
    uaddr stackTop() const { return m_StackTop; }

    Expected<uaddr> virtualToHost(uaddr vaddr) const;

    /* Import Lookup Table */

    Expected<void> allocateILT(usize numEntries);
    uaddr insertILTEntry(const std::string& symbolName);
};

} // namespace dashle::guest::arm

#endif /* _DASHLE_GUEST_ARM_ENVIRONMENT_H */