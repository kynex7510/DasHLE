#include "DasHLE/Emu/ARM/EmuContext.h"

using namespace dashle;
using namespace dashle::emu::arm;

constexpr static usize STACK_SIZE = 1024 * 1024; // 1MB

static u32 cpsrThumbEnable(u32 cpsr) { return cpsr | 0x30u; }
static u32 cpsrThumbDisable(u32 cpsr) { return cpsr & ~(0x30u); }
static bool isThumb(uaddr addr) { return addr & 1u; }
static uaddr clearThumb(uaddr addr) { return addr & ~(1u); }

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

    return cfg;
}

EmuContext::EmuContext() { m_Env = std::make_unique<DefaultEnvironment>(STACK_SIZE); }

Expected<void> EmuContext::openBinary(const host::fs::path& path) { return m_Env->openBinary(path); }

void EmuContext::initCpu() {
    if (m_Jit)
        return;

    m_Jit = std::make_unique<dynarmic32::Jit>(buildConfig());

    // Setup stack.
    const auto stackTop = m_Env->stackTop();
    if (stackTop)
        setRegister(REG_SP, stackTop.value());

    // Run initializers.
    for (auto vaddr : m_Env->initializers()) {
        if (auto reason = execute(vaddr); static_cast<u32>(reason) != 0) {
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
    setRegister(REG_PC, clearThumb(addr));
    setThumb(isThumb(addr));
    return m_Jit->Run();
}

void EmuContext::dump() {
    constexpr const char* REGS[] = {
        "R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7",
        "R8", "R9", "R10", "R11", "R12", "SP", "LR", "PC"
    };

    DASHLE_LOG_LINE("=== CONTEXT DUMP ===");
    for (auto i = 0; i < 16; ++i)
        DASHLE_LOG_LINE("{}: 0x{:08X}", REGS[i], getRegister(i));

    DASHLE_LOG_LINE("CPSR: 0x{:08X}", m_Jit->Cpsr());
    DASHLE_LOG_LINE("FSPCR: 0x{:08X}", m_Jit->Fpscr());
}