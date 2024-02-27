#include "DasHLE/Support/Math.h"
#include "DasHLE/Guest/ARM/ARM.h"

using namespace dashle;
using namespace dashle::guest;
using namespace dashle::guest::arm;

// START DEBUG
static std::string getPermString(usize flags) {
    std::string permString("---");

    if (flags & host::memory::flags::PERM_READ)
        permString[0] = 'r';

    if (flags & host::memory::flags::PERM_WRITE)
        permString[1] = 'w';

    if (flags & host::memory::flags::PERM_EXEC)
        permString[2] = 'x';

    return permString;
}
// END DEBUG

// Environment

struct ARMVM::Environment final : public dynarmic32::UserCallbacks {
    std::shared_ptr<host::memory::MemoryManager> m_Mem;
    std::shared_ptr<host::bridge::Bridge> m_Bridge;

    Environment(std::shared_ptr<host::memory::MemoryManager> mem, std::shared_ptr<host::bridge::Bridge> bridge)
        : m_Mem(mem), m_Bridge(bridge) {
        DASHLE_ASSERT(m_Mem);
        DASHLE_ASSERT(m_Bridge);
    }
    
    virtual ~Environment() noexcept {}

    Expected<uaddr> virtualToHost(uaddr vaddr) const {
        DASHLE_ASSERT(m_Mem);
        return m_Mem->blockFromVAddr(vaddr).and_then([vaddr](const host::memory::AllocatedBlock* block) {
            return host::memory::virtualToHost(*block, vaddr);
        });
    }

    Expected<uaddr> virtualToHostChecked(uaddr vaddr, usize flags) const {
        constexpr bool verbose = true;
        if constexpr(dashle::DEBUG_MODE) {
            flags &= host::memory::flags::PERM_MASK;
            const auto block = m_Mem->blockFromVAddr(vaddr);
            if (verbose && !block) {
                DASHLE_LOG_LINE("Block not found (vaddr=0x{:X}, expected={})", vaddr, getPermString(flags));
                DASHLE_LOG_LINE("Gonna assert wawa");
            }
            DASHLE_ASSERT(block);
            if (verbose && !(block.value()->flags & flags)) {
                DASHLE_LOG_LINE("Invalid flags (expected={}, found={})", getPermString(flags), getPermString(block.value()->flags));
                DASHLE_LOG_LINE("Trying to read 0x{:X}", vaddr);
                DASHLE_LOG_LINE("Gonna assert wawa");
            }
            DASHLE_ASSERT(block.value()->flags & flags);
        }

        return virtualToHost(vaddr);
    }

    /* Dynarmic callbacks */

    bool PreCodeReadHook(bool isThumb, dynarmic32::VAddr pc, dynarmic32::IREmitter& ir) override {
        return !m_Bridge->emitCall(pc, &ir);
    }

    std::optional<std::uint32_t> MemoryReadCode(dynarmic32::VAddr vaddr) override {
        const auto hostAddr = virtualToHostChecked(vaddr, host::memory::flags::PERM_EXEC);
        if (hostAddr)
            return *reinterpret_cast<const std::uint32_t*>(hostAddr.value());

        return {};
    }

    std::uint8_t MemoryRead8(dynarmic32::VAddr vaddr) override {
        DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_READ));
        return *reinterpret_cast<const std::uint8_t*>(addr);
    }

    std::uint16_t MemoryRead16(dynarmic32::VAddr vaddr) override {
        DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_READ));
        return *reinterpret_cast<const std::uint16_t*>(addr);
    }

    std::uint32_t MemoryRead32(dynarmic32::VAddr vaddr) override {
        DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_READ));
        return *reinterpret_cast<const std::uint32_t*>(addr);
    }

    std::uint64_t MemoryRead64(dynarmic32::VAddr vaddr) override {
        DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_READ));
        return *reinterpret_cast<const std::uint64_t*>(addr);
    }

    void MemoryWrite8(dynarmic32::VAddr vaddr, std::uint8_t value) override {
        DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_WRITE));
        *reinterpret_cast<std::uint8_t*>(addr) = value;
    }

    void MemoryWrite16(dynarmic32::VAddr vaddr, std::uint16_t value) override {
        DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_WRITE));
        *reinterpret_cast<std::uint16_t*>(addr) = value;
    }

    void MemoryWrite32(dynarmic32::VAddr vaddr, std::uint32_t value) override {
        DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_WRITE));
        *reinterpret_cast<std::uint32_t*>(addr) = value;
    }
        
    void MemoryWrite64(dynarmic32::VAddr vaddr, std::uint64_t value) override {
        DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_WRITE));
        *reinterpret_cast<std::uint64_t*>(addr) = value;
    }

    bool MemoryWriteExclusive8(dynarmic32::VAddr vaddr, std::uint8_t value, std::uint8_t expected) override {
        // TODO
        MemoryWrite8(vaddr, value);
        return true;
    }

    bool MemoryWriteExclusive16(dynarmic32::VAddr vaddr, std::uint16_t value, std::uint16_t expected) override {
        // TODO
        MemoryWrite16(vaddr, value);
        return true;
    }

    bool MemoryWriteExclusive32(dynarmic32::VAddr vaddr, std::uint32_t value, std::uint32_t expected) override {
        // TODO
        MemoryWrite32(vaddr, value);
        return true;
    }

    bool MemoryWriteExclusive64(dynarmic32::VAddr vaddr, std::uint64_t value, std::uint64_t expected) override {
        // TODO
        MemoryWrite64(vaddr, value);
        return true;
    }

    bool IsReadOnlyMemory(dynarmic32::VAddr vaddr) override {
        DASHLE_ASSERT(m_Mem);
        if (auto block = m_Mem->blockFromVAddr(vaddr))
            return !(block.value()->flags & host::memory::flags::PERM_WRITE);
                
        return false;
    }

    void InterpreterFallback(dynarmic32::VAddr pc, usize numInstructions) override {
        DASHLE_UNREACHABLE("Interpreter invoked (pc={}, numInstructions={})", pc, numInstructions);
    }

    void CallSVC(std::uint32_t swi) override {
        DASHLE_UNREACHABLE("Unimplemented syscall (swi={})", swi);
    }

    void ExceptionRaised(dynarmic32::VAddr pc, dynarmic32::Exception exception) override {
        // TODO: handle exceptions.
        DASHLE_UNREACHABLE("Unimplemented exception handling (pc=0x{:X}, exception={})", pc, static_cast<u32>(exception));
    }

    void AddTicks(std::uint64_t ticks) override {}
    std::uint64_t GetTicksRemaining() override { return static_cast<u64>(-1); }
};

// ARMVM

constexpr static u32 cpsrThumbEnable(u32 cpsr) { return cpsr | 0x30u; }
constexpr static u32 cpsrThumbDisable(u32 cpsr) { return cpsr & ~(0x30u); }
constexpr static bool isThumb(u32 addr) { return addr & 1u; }
constexpr static u32 clearThumb(u32 addr) { return addr & ~(1u); }

void ARMVM::setPC(uaddr addr) {
    DASHLE_ASSERT(m_Jit);
    const auto cpsr = m_Jit->Cpsr();
    m_Jit->SetCpsr(isThumb(addr) ? cpsrThumbEnable(cpsr) : cpsrThumbDisable(cpsr));
    m_Jit->Regs()[regs::PC] = clearThumb(addr);
}

ARMVM::ARMVM(std::shared_ptr<host::memory::MemoryManager> mem, std::shared_ptr<host::bridge::Bridge> bridge, GuestVersion version)
    : m_Mem(mem) {
    DASHLE_ASSERT(m_Mem);

    // Get special address used to know when to terminate execution.
    DASHLE_ASSERT_WRAPPER_CONST(block, m_Mem->allocate({
        .size = 4u, // Size of an ARM instruction
        .alignment = 4u, // Aligned for a correct PC value
        .flags = 0u, // Must not be accessible
    }));
    m_EndExecVAddr = block->virtualBase;

    // Create environment.
    m_Env = std::make_unique<ARMVM::Environment>(mem, bridge);

    // Create exclusive monitor.
    m_ExMon = std::make_unique<dynarmic::ExclusiveMonitor>(1);

    // Build config.
    dynarmic32::UserConfig cfg;
    cfg.callbacks = m_Env.get();

    switch (version) {
        case GuestVersion::Armeabi:
            cfg.arch_version = dynarmic32::ArchVersion::v5TE;
            break;
        case GuestVersion::Armeabi_v7a:
            cfg.arch_version = dynarmic32::ArchVersion::v7;
            break;
        default:
            DASHLE_UNREACHABLE("Invalid guest version!");
    }

    if constexpr(dashle::DEBUG_MODE)
        cfg.optimizations = dynarmic::no_optimizations;
    else
        cfg.optimizations = dynarmic::all_safe_optimizations;

    cfg.global_monitor = m_ExMon.get();

    // Create jit.
    m_Jit = std::make_unique<dynarmic32::Jit>(cfg);
}

ARMVM::~ARMVM() { DASHLE_ASSERT(m_Mem->free(m_EndExecVAddr)); }

dynarmic::HaltReason ARMVM::execute(Optional<uaddr> wrappedAddr) {
    DASHLE_ASSERT(m_Jit);

    if (!wrappedAddr)
        return m_Jit->Run();

    DASHLE_ASSERT_WRAPPER_CONST(addr, wrappedAddr);
    m_Jit->Regs()[regs::LR] = m_EndExecVAddr;
    setPC(addr);

    auto reason = VM_EXEC_SUCCESS;
    while (reason == VM_EXEC_SUCCESS) {
        reason = m_Jit->Run();
        if (m_Jit->Regs()[regs::PC] == m_EndExecVAddr)
            break;

        DASHLE_LOG_LINE("PC: 0x{:X}, LR: 0x{:X}", m_Jit->Regs()[regs::PC], m_Jit->Regs()[regs::LR]);
    }

    return reason;
}

dynarmic::HaltReason ARMVM::step(Optional<uaddr> wrappedAddr) {
    DASHLE_ASSERT(m_Jit);

    if (wrappedAddr) {
        DASHLE_ASSERT_WRAPPER_CONST(addr, wrappedAddr);
        setPC(addr);
    }

    return m_Jit->Step();
}

void ARMVM::setRegister(usize id, u64 value) {
    DASHLE_ASSERT(m_Jit);

    if (id <= regs::R15) {
        m_Jit->Regs()[id] = value;
        return;
    }

    switch (id) {
        case regs::CPSR:
            m_Jit->SetCpsr(value);
            return;
        case regs::FPSCR:
            m_Jit->SetFpscr(value);
            return;
    }

    DASHLE_UNREACHABLE("Invalid ID!");
}

u64 ARMVM::getRegister(usize id) const {
    DASHLE_ASSERT(m_Jit);

    if (id <= regs::R15)
        return m_Jit->Regs()[id];

    switch (id) {
        case regs::CPSR:
            return m_Jit->Cpsr();
        case regs::FPSCR:
            return m_Jit->Fpscr();
    }

    DASHLE_UNREACHABLE("Invalid ID!");
}

void ARMVM::dumpContext() const {
    if constexpr(dashle::DEBUG_MODE) {
        DASHLE_ASSERT(m_Jit);

        constexpr const char* LABELS[] = {
            "R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7",
            "R8", "R9", "R10", "R11 (FP)", "R12 (IP)", "R13 (SP)", "R14 (LR)", "R15 (PC)"
        };

        DASHLE_LOG_LINE("=== CONTEXT DUMP ===");
        for (auto i = 0; i < 16; ++i)
            DASHLE_LOG_LINE("{}: 0x{:08X}", LABELS[i], m_Jit->Regs()[i]);

        DASHLE_LOG_LINE("CPSR: 0x{:08X}", m_Jit->Cpsr());
        DASHLE_LOG_LINE("FSPCR: 0x{:08X}", m_Jit->Fpscr());
    }
}