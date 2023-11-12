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

    // Set stack base.
    const auto stackBase = m_Env->stackBase();
    if (stackBase)
        setRegister(REG_SP, stackBase.value());
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

void EmuContext::execute(uaddr addr) {
    setRegister(REG_PC, clearThumb(addr));
    setThumb(isThumb(addr));
    auto const reason = m_Jit->Run();
    DASHLE_LOG_LINE("Halted with reason = {}", static_cast<u32>(reason));
}