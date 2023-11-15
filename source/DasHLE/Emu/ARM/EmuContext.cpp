#include "DasHLE/Emu/ARM/EmuContext.h"

#include "dynarmic/interface/exclusive_monitor.h"

using namespace dashle;
using namespace dashle::emu::arm;

constexpr static usize STACK_SIZE = 1024 * 1024; // 1MB
constexpr static auto NO_REASON = static_cast<dynarmic::HaltReason>(0u);

constexpr static u32 cpsrThumbEnable(u32 cpsr) { return cpsr | 0x30u; }
constexpr static u32 cpsrThumbDisable(u32 cpsr) { return cpsr & ~(0x30u); }
constexpr static bool isThumb(uaddr addr) { return addr & 1u; }
constexpr static uaddr clearThumb(uaddr addr) { return addr & ~(1u); }

// EmuContext

dynarmic32::UserConfig EmuContext::buildConfig() const {
    dynarmic32::UserConfig cfg;
    cfg.callbacks = m_Env.get();

    DASHLE_ASSERT_WRAPPER_CONST(binaryVersion, m_Env->binaryVersion());
    switch (binaryVersion) {
        case BinaryVersion::Armeabi:
            cfg.arch_version = dynarmic32::ArchVersion::v5TE;
            break;
        case BinaryVersion::Armeabi_v7a:
            cfg.arch_version = dynarmic32::ArchVersion::v7;
            break;
        default:
            // TODO: formatter
            DASHLE_UNREACHABLE("Unknown binary type (type={})", static_cast<u32>(binaryVersion));
    }

    if constexpr(DEBUG_MODE)
        cfg.optimizations = dynarmic::no_optimizations;
    else
        cfg.optimizations = dynarmic::all_safe_optimizations;

    //
    cfg.global_monitor = new dynarmic::ExclusiveMonitor(1);
    //

    return cfg;
}

EmuContext::EmuContext() { m_Env = std::make_unique<DefaultEnvironment>(STACK_SIZE); }

Expected<void> EmuContext::openBinary(const host::fs::path& path) { return m_Env->openBinary(path); }

void EmuContext::initCpu() {
    if (m_Jit)
        return;

    // Create JIT.
    m_Jit = std::make_unique<dynarmic32::Jit>(buildConfig());

    // Setup stack.
    const auto stackTop = m_Env->stackTop();
    if (stackTop)
        setRegister(REG_SP, stackTop.value());

    // Run initializers.
    const auto addr = m_Env->initializers()[0];
    setRegister(REG_PC, clearThumb(addr));
    setThumb(isThumb(addr));

    for (auto vaddr : m_Env->initializers()) {
        DASHLE_LOG_LINE("Calling initializer @ 0x{:X}...", vaddr);
        if (auto reason = execute(vaddr); reason != dynarmic::HaltReason::Step) {
            DASHLE_LOG_LINE("Init call failed (vaddr={}, reason={})", vaddr, static_cast<u32>(reason));
            return;
        }
    }
}

void EmuContext::setRegister(usize index, usize value) {
    DASHLE_ASSERT(m_Jit);
    DASHLE_ASSERT(index < 16);
    m_Jit->Regs()[index] = value;
}

usize EmuContext::getRegister(usize index) const {
    DASHLE_ASSERT(m_Jit);
    DASHLE_ASSERT(index < 16);
    return m_Jit->Regs()[index];
}

void EmuContext::setThumb(bool thumb) {
    DASHLE_ASSERT(m_Jit);
    const auto cpsr = m_Jit->Cpsr();
    m_Jit->SetCpsr(thumb ? cpsrThumbEnable(cpsr) : cpsrThumbDisable(cpsr));
}

dynarmic::HaltReason EmuContext::execute(uaddr addr) {
    constexpr static uaddr STOP_ADDR = clearThumb(0xDEADC0DE);

    setRegister(REG_LR, STOP_ADDR);
    setRegister(REG_PC, clearThumb(addr));
    setThumb(isThumb(addr));

    dynarmic::HaltReason reason = NO_REASON;
    while (reason == NO_REASON) {
        reason = m_Jit->Run();
        //DASHLE_LOG_LINE("PC: 0x{:X}", getRegister(REG_PC));
        if (getRegister(REG_PC) == STOP_ADDR)
            break;
    }

    return reason;
}

void EmuContext::dump() {
    constexpr const char* LABELS[] = {
        "R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7",
        "R8", "R9", "R10", "R11 (FP)", "R12 (IP)", "R13 (SP)", "R14 (LR)", "R15 (PC)"
    };

    DASHLE_LOG_LINE("=== CONTEXT DUMP ===");
    for (auto i = 0; i < 16; ++i)
        DASHLE_LOG_LINE("{}: 0x{:08X}", LABELS[i], getRegister(i));

    DASHLE_LOG_LINE("CPSR: 0x{:08X}", m_Jit->Cpsr());
    DASHLE_LOG_LINE("FSPCR: 0x{:08X}", m_Jit->Fpscr());
}