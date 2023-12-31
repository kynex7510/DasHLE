#include "DasHLE/Binary/Binary.h"
#include "DasHLE/Guest/ARM.h"

using namespace dashle;
using namespace dashle::guest;
using namespace dashle::guest::arm;

struct ELFConfig : binary::elf::Config32, binary::elf::ConfigLE {
    constexpr static auto ARCH = binary::elf::constants::EM_ARM;
};

using Binary = binary::Binary<ELFConfig>;

constexpr static u32 cpsrThumbEnable(u32 cpsr) { return cpsr | 0x30u; }
constexpr static u32 cpsrThumbDisable(u32 cpsr) { return cpsr & ~(0x30u); }
constexpr static bool isThumb(u32 addr) { return addr & 1u; }
constexpr static u32 clearThumb(u32 addr) { return addr & ~(1u); }

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

// Environment

struct VMImpl::Environment final : public dynarmic32::UserCallbacks {
    std::shared_ptr<host::memory::MemoryManager> m_Mem;
    std::shared_ptr<host::interop::InteropHandler> m_Interop;

    Environment(std::shared_ptr<host::memory::MemoryManager> mem, std::shared_ptr<host::interop::InteropHandler> interop)
        : m_Mem(mem), m_Interop(interop) {}
    
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
        return !m_Interop->invokeEmitterCallback(pc, &ir);
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

// VMImpl

void VMImpl::setupJit(binary::Version version) {
    // Build config.
    dynarmic32::UserConfig cfg;
    cfg.callbacks = m_Env.get();

    switch (version) {
        using enum binary::Version;

    case Armeabi:
        cfg.arch_version = dynarmic32::ArchVersion::v5TE;
        break;
    case Armeabi_v7a:
        cfg.arch_version = dynarmic32::ArchVersion::v7;
        break;
    default:
        DASHLE_UNREACHABLE("Invalid binary version!");
    }

    if constexpr(dashle::DEBUG_MODE)
        cfg.optimizations = dynarmic::no_optimizations;
    else
        cfg.optimizations = dynarmic::all_safe_optimizations;

    m_ExMon->Clear();
    cfg.global_monitor = m_ExMon.get();

    // Create jit.
    m_Jit = std::move(std::make_unique<dynarmic32::Jit>(cfg));
    m_Jit->Regs()[regs::SP] = m_StackTop;
}

void VMImpl::setPC(uaddr addr) {
    DASHLE_ASSERT(m_Jit);
    const auto cpsr = m_Jit->Cpsr();
    m_Jit->SetCpsr(isThumb(addr) ? cpsrThumbEnable(cpsr) : cpsrThumbDisable(cpsr));
    m_Jit->Regs()[regs::PC] = clearThumb(addr);
}

VMImpl::VMImpl(std::shared_ptr<host::memory::MemoryManager> mem, std::shared_ptr<host::interop::InteropHandler> interop,
    const host::interop::SymResolver* resolver) {
    DASHLE_ASSERT(resolver);

    m_SymResolver = resolver;
    m_Env = std::make_unique<VMImpl::Environment>(mem, interop);
    m_ExMon = std::make_unique<dynarmic::ExclusiveMonitor>(1);

    // Allocate stack.
    DASHLE_ASSERT_WRAPPER_CONST(block, mem->allocate({
        .size = STACK_SIZE,
        .alignment = PAGE_SIZE,
        .hint = mem->virtualOffset() + mem->maxMemory() - STACK_SIZE,
        .flags = host::memory::flags::PERM_READ_WRITE | host::memory::flags::FORCE_HINT
    }));
    m_StackBase = block->virtualBase;
    m_StackTop = m_StackBase + block->size;
}

VMImpl::~VMImpl() {
    auto mem = m_Env->m_Mem;
    DASHLE_ASSERT(mem->free(m_StackBase));
}

struct LoadSegmentInfo {
    usize fileDataOffset; // Offset in the file for the first byte of this segment.
    usize fileDataSize; // Size of the data to read from file.
    usize memDataOffset; // Offset in memory for the first byte of this segment.
    usize allocOffset; // Offset in memory where this segment starts.
    usize allocSize; // Size for the memory allocation.
    usize permissions; // Permission flags.
};

Expected<void> VMImpl::loadBinary(const std::span<const u8> buffer) {
    Binary binary(buffer);
    auto mem = m_Env->m_Mem;

    // Get ELF LOAD segments.
    std::vector<LoadSegmentInfo> loadSegmentsInfo;
    DASHLE_TRY_EXPECTED_CONST(loadSegments, binary.segments(binary::elf::PT_LOAD));
    usize binaryAllocSize = 0u;

    for (const auto segment : loadSegments) {
        // p_vaddr is the offset in memory for the first byte of the segment.
        // allocOffset is p_vaddr but aligned down, so the offset in memory for the segment start.
        DASHLE_TRY_EXPECTED_CONST(allocOffset, binary.segmentAllocOffset(segment));
        DASHLE_TRY_EXPECTED_CONST(allocSize, binary.segmentAllocSize(segment));
        binaryAllocSize += allocSize;
        loadSegmentsInfo.emplace_back(LoadSegmentInfo {
            .fileDataOffset = segment->p_offset,
            .fileDataSize = segment->p_filesz,
            .memDataOffset = segment->p_vaddr - allocOffset,
            .allocOffset = allocOffset,
            .allocSize = allocSize,
            .permissions = binary.wrapPermissionFlags(segment->p_flags)
        });
    }

    // Get binary base.
    DASHLE_TRY_EXPECTED_CONST(binaryBase, mem->findFreeAddr(binaryAllocSize, PAGE_SIZE));
    DASHLE_LOG_LINE("Binary base: 0x{:X}", binaryBase);

    // Allocate and map each segment.
    for (const auto& segmentInfo : loadSegmentsInfo) {
        DASHLE_TRY_EXPECTED_CONST(block, mem->allocate({
            .size = segmentInfo.allocSize,
            .alignment = PAGE_SIZE,
            .hint = binaryBase + segmentInfo.allocOffset,
            .flags = host::memory::flags::PERM_READ_WRITE | host::memory::flags::FORCE_HINT,
        }));

        std::copy(buffer.data() + segmentInfo.fileDataOffset,
            buffer.data() + segmentInfo.fileDataOffset + segmentInfo.fileDataSize,
            reinterpret_cast<u8*>(block->hostBase) + segmentInfo.memDataOffset);
    }

    // Handle relocations.
    const auto& relocs = binary.relocs();

    for (const auto& reloc : binary.relocs()) {
        DASHLE_TRY_EXPECTED_CONST(patchAddr, virtualToHost(binaryBase + reloc.patchOffset));

        if (reloc.kind == binary::RelocKind::Relative) {
            if (reloc.addend)
                *reinterpret_cast<ELFConfig::AddrType*>(patchAddr) = binaryBase + reloc.addend;
            else
                *reinterpret_cast<ELFConfig::AddrType*>(patchAddr) += binaryBase;
            continue;
        }

        if (reloc.kind == binary::RelocKind::Symbol) {
            DASHLE_ASSERT_WRAPPER_CONST(symbolName, reloc.symbolName);
            DASHLE_TRY_EXPECTED_CONST(vaddr, m_SymResolver->resolve(symbolName));
            // We ignore the addend.
            *reinterpret_cast<ELFConfig::AddrType*>(patchAddr) = vaddr;
            continue;
        }

        DASHLE_UNREACHABLE("Invalid relocation kind!");
    }

    // Set memory permissions.
    for (const auto& segmentInfo : loadSegmentsInfo) {
        DASHLE_TRY_EXPECTED_VOID(mem->setFlags(binaryBase + segmentInfo.allocOffset, segmentInfo.permissions));
    }

    // Get initializers and finalizers.
    const auto initWrapper = binary.initArrayInfo();
    if (initWrapper) {
        const auto& initArrayInfo = initWrapper.value();
        DASHLE_TRY_EXPECTED_CONST(hostAddr, virtualToHost(binaryBase + initArrayInfo.offset));
        const auto initArray = reinterpret_cast<const ELFConfig::AddrType*>(hostAddr);
        m_Initializers.clear();
        for (auto i = 0u; i < initArrayInfo.size; ++i) {
            const auto addr = initArray[i];
            if (addr != 0 && addr != -1)
                m_Initializers.push_back(addr);
        }
    }

    const auto finiWrapper = binary.finiArrayInfo();
    if (finiWrapper) {
        const auto& finiArrayInfo = finiWrapper.value();
        DASHLE_TRY_EXPECTED_CONST(hostAddr, virtualToHost(binaryBase + finiArrayInfo.offset));
        const auto finiArray = reinterpret_cast<const ELFConfig::AddrType*>(hostAddr);
        m_Finalizers.clear();
        for (auto i = 0u; i < finiArrayInfo.size; ++i) {
            const auto addr = finiArray[i];
            if (addr != 0 && addr != -1)
                m_Finalizers.push_back(finiArray[i]);
        }
    }

    setupJit(binary::Version::Armeabi_v7a); // TODO
    return EXPECTED_VOID;
}

Expected<uaddr> VMImpl::virtualToHost(uaddr vaddr) const {
    return m_Env->virtualToHost(vaddr);
}

dynarmic::HaltReason VMImpl::execute(Optional<uaddr> wrappedAddr) {
    DASHLE_ASSERT(m_Jit);

    if (!wrappedAddr)
        return m_Jit->Run();

    const auto stopAddr = m_StackBase;
    DASHLE_ASSERT_WRAPPER_CONST(addr, wrappedAddr);
    m_Jit->Regs()[regs::LR] = stopAddr;
    setPC(addr);

    auto reason = EXEC_SUCCESS;
    while (reason == EXEC_SUCCESS) {
        reason = m_Jit->Run();
        if (m_Jit->Regs()[regs::PC] == stopAddr)
            break;

        DASHLE_LOG_LINE("PC: 0x{:X}, LR: 0x{:X}", m_Jit->Regs()[regs::PC], m_Jit->Regs()[regs::LR]);
    }

    return reason;
}

dynarmic::HaltReason VMImpl::step(Optional<uaddr> wrappedAddr) {
    DASHLE_ASSERT(m_Jit);

    if (wrappedAddr) {
        DASHLE_ASSERT_WRAPPER_CONST(addr, wrappedAddr);
        setPC(addr);
    }

    return m_Jit->Step();
}

void VMImpl::runInitializers() {
    for (auto vaddr : m_Initializers) {
        if (auto reason = execute(vaddr); reason != EXEC_SUCCESS) {
            DASHLE_UNREACHABLE("Init call failed (vaddr=0x{:X}, reason={})", vaddr, static_cast<u32>(reason));
        }
    }
}

void VMImpl::setRegister(usize id, u64 value) {
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

u64 VMImpl::getRegister(usize id) const {
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

void VMImpl::dump() const {
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