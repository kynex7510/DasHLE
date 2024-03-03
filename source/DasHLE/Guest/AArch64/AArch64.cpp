#include "DasHLE/Support/Math.h"
#include "DasHLE/Guest/AArch64/AArch64.h"

using namespace dashle;
using namespace dashle::guest;
using namespace dashle::guest::aarch64;

constexpr static usize PAGE_SIZE = 0x1000; // 4KB
static_assert(dashle::isPowerOfTwo(PAGE_SIZE));

constexpr static usize STACK_SIZE = 1024 * 1024; // 1MB
static_assert(dashle::alignDown<usize>(STACK_SIZE, PAGE_SIZE) == STACK_SIZE);

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

struct AArch64VM::Environment final : public dynarmic64::UserCallbacks {
    std::shared_ptr<host::memory::MemoryManager> m_Mem;
    std::shared_ptr<host::bridge::Bridge> m_Bridge;

    Environment(std::shared_ptr<host::memory::MemoryManager> mem, std::shared_ptr<host::bridge::Bridge> bridge)
        : m_Mem(mem), m_Bridge(bridge) {}
    
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

    /*
    bool PreCodeReadHook(bool isThumb, dynarmic32::VAddr pc, dynarmic32::IREmitter& ir) override {
        return !m_Bridge->emitCall(pc, &ir);
    }
    */

    std::optional<std::uint32_t> MemoryReadCode(dynarmic64::VAddr vaddr) override {
        const auto hostAddr = virtualToHostChecked(vaddr, host::memory::flags::PERM_EXEC);
        if (hostAddr)
            return *reinterpret_cast<const std::uint32_t*>(hostAddr.value());

        return {};
    }

    std::uint8_t MemoryRead8(dynarmic64::VAddr vaddr) override {
        DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_READ));
        return *reinterpret_cast<const std::uint8_t*>(addr);
    }

    std::uint16_t MemoryRead16(dynarmic64::VAddr vaddr) override {
        DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_READ));
        return *reinterpret_cast<const std::uint16_t*>(addr);
    }

    std::uint32_t MemoryRead32(dynarmic64::VAddr vaddr) override {
        DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_READ));
        return *reinterpret_cast<const std::uint32_t*>(addr);
    }

    std::uint64_t MemoryRead64(dynarmic64::VAddr vaddr) override {
        DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_READ));
        return *reinterpret_cast<const std::uint64_t*>(addr);
    }

    void MemoryWrite8(dynarmic64::VAddr vaddr, std::uint8_t value) override {
        DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_WRITE));
        *reinterpret_cast<std::uint8_t*>(addr) = value;
    }

    void MemoryWrite16(dynarmic64::VAddr vaddr, std::uint16_t value) override {
        DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_WRITE));
        *reinterpret_cast<std::uint16_t*>(addr) = value;
    }

    void MemoryWrite32(dynarmic64::VAddr vaddr, std::uint32_t value) override {
        DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_WRITE));
        *reinterpret_cast<std::uint32_t*>(addr) = value;
    }
        
    void MemoryWrite64(dynarmic64::VAddr vaddr, std::uint64_t value) override {
        DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_WRITE));
        *reinterpret_cast<std::uint64_t*>(addr) = value;
    }

    bool MemoryWriteExclusive8(dynarmic64::VAddr vaddr, std::uint8_t value, std::uint8_t expected) override {
        // TODO
        MemoryWrite8(vaddr, value);
        return true;
    }

    bool MemoryWriteExclusive16(dynarmic64::VAddr vaddr, std::uint16_t value, std::uint16_t expected) override {
        // TODO
        MemoryWrite16(vaddr, value);
        return true;
    }

    bool MemoryWriteExclusive32(dynarmic64::VAddr vaddr, std::uint32_t value, std::uint32_t expected) override {
        // TODO
        MemoryWrite32(vaddr, value);
        return true;
    }

    bool MemoryWriteExclusive64(dynarmic64::VAddr vaddr, std::uint64_t value, std::uint64_t expected) override {
        // TODO
        MemoryWrite64(vaddr, value);
        return true;
    }

    bool IsReadOnlyMemory(dynarmic64::VAddr vaddr) override {
        DASHLE_ASSERT(m_Mem);
        if (auto block = m_Mem->blockFromVAddr(vaddr))
            return !(block.value()->flags & host::memory::flags::PERM_WRITE);
                
        return false;
    }

    void InterpreterFallback(dynarmic64::VAddr pc, usize numInstructions) override {
        DASHLE_UNREACHABLE("Interpreter invoked (pc={}, numInstructions={})", pc, numInstructions);
    }

    void CallSVC(std::uint32_t swi) override {
        DASHLE_UNREACHABLE("Unimplemented syscall (swi={})", swi);
    }

    void ExceptionRaised(dynarmic64::VAddr pc, dynarmic64::Exception exception) override {
        // TODO: handle exceptions.
        DASHLE_UNREACHABLE("Unimplemented exception handling (pc=0x{:X}, exception={})", pc, static_cast<u32>(exception));
    }

    void AddTicks(std::uint64_t ticks) override {}
    std::uint64_t GetTicksRemaining() override { return static_cast<u64>(-1); }
};

// AArch64VM

AArch64VM::AArch64VM(std::shared_ptr<host::memory::MemoryManager> mem, std::shared_ptr<host::bridge::Bridge> bridge)
    : StackVM(mem, STACK_SIZE, PAGE_SIZE) {
    m_Env = std::make_unique<AArch64VM::Environment>(mem, bridge);
    m_ExMon = std::make_unique<dynarmic::ExclusiveMonitor>(1);
}

void AArch64VM::setupJit() {
    // Build config.
    dynarmic64::UserConfig cfg;
    cfg.callbacks = m_Env.get();

    if constexpr(dashle::DEBUG_MODE)
        cfg.optimizations = dynarmic::no_optimizations;
    else
        cfg.optimizations = dynarmic::all_safe_optimizations;

    m_ExMon->Clear();
    cfg.global_monitor = m_ExMon.get();

    // Create jit.
    m_Jit = std::move(std::make_unique<dynarmic64::Jit>(cfg));
    m_Jit->SetSP(m_StackTop);
}

dynarmic::HaltReason AArch64VM::execute(Optional<uaddr> wrappedAddr) {
    DASHLE_ASSERT(m_Jit);

    if (!wrappedAddr)
        return m_Jit->Run();

    const auto stopAddr = m_StackBase;
    DASHLE_ASSERT_WRAPPER_CONST(addr, wrappedAddr);
    // m_Jit->Regs()[regs::LR] = stopAddr;
    m_Jit->SetPC(addr);

    auto reason = EXEC_SUCCESS;
    while (reason == EXEC_SUCCESS) {
        reason = m_Jit->Run();
        if (m_Jit->GetPC() == stopAddr)
            break;

        // DASHLE_LOG_LINE("PC: 0x{:X}, LR: 0x{:X}", m_Jit->Regs()[regs::PC], m_Jit->Regs()[regs::LR]);
    }

    return reason;
}

dynarmic::HaltReason AArch64VM::step(Optional<uaddr> wrappedAddr) {
    DASHLE_ASSERT(m_Jit);

    if (wrappedAddr) {
        DASHLE_ASSERT_WRAPPER_CONST(addr, wrappedAddr);
        m_Jit->SetPC(addr);
    }

    return m_Jit->Step();
}

void AArch64VM::setRegister(usize id, u64 value) {
    DASHLE_ASSERT(m_Jit);

    /*
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
    */
}

u64 AArch64VM::getRegister(usize id) const {
    DASHLE_ASSERT(m_Jit);

    /*
    if (id <= regs::R15)
        return m_Jit->Regs()[id];

    switch (id) {
        case regs::CPSR:
            return m_Jit->Cpsr();
        case regs::FPSCR:
            return m_Jit->Fpscr();
    }

    DASHLE_UNREACHABLE("Invalid ID!");
    */
}