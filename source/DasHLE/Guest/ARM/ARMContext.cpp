#include "DasHLE/Guest/ARM/Context.h"

using namespace dashle;
using namespace dashle::guest;
using namespace dashle::guest::arm;

using Binary = binary::Binary<arm::ELFConfig>;

constexpr static usize STACK_SIZE = 1024 * 1024; // 1MB

constexpr static u32 cpsrThumbEnable(u32 cpsr) { return cpsr | 0x30u; }
constexpr static u32 cpsrThumbDisable(u32 cpsr) { return cpsr & ~(0x30u); }
constexpr static bool isThumb(uaddr addr) { return addr & 1u; }
constexpr static uaddr clearThumb(uaddr addr) { return addr & ~(1u); }

static std::unique_ptr<host::memory::MemoryManager> makeDefaultMem() {
    return std::make_unique<host::memory::MemoryManager>(
        std::make_unique<host::memory::HostAllocator>(),
        static_cast<usize>(1u) << 32);
}

// ARMContext

void ARMContext::initJit() {
    auto env = environment();

    env->m_Ctx = this;

    // Build config.
    dynarmic32::UserConfig cfg;
    cfg.callbacks = env;

    // TODO
    cfg.arch_version = dynarmic32::ArchVersion::v7;

    if constexpr(DEBUG_MODE)
        cfg.optimizations = dynarmic::no_optimizations;
    else
        cfg.optimizations = dynarmic::all_safe_optimizations;

    m_ExMon->Clear();
    cfg.global_monitor = m_ExMon.get();

    // Create jit.
    m_Jit = std::move(std::make_unique<dynarmic32::Jit>(cfg));
    m_Jit->Regs()[regs::SP] = env->stackTop();

    // Run initializers.
    for (auto vaddr : m_Initializers) {
        if (auto reason = execute(vaddr); reason != EXEC_SUCCESS) {
            DASHLE_UNREACHABLE("Init call failed (vaddr=0x{:X}, reason={})", vaddr, static_cast<u32>(reason));
        }
    }

    return;
}

void ARMContext::setPC(uaddr addr) {
    DASHLE_ASSERT(m_Jit);
    const auto cpsr = m_Jit->Cpsr();
    m_Jit->SetCpsr(isThumb(addr) ? cpsrThumbEnable(cpsr) : cpsrThumbDisable(cpsr));
    m_Jit->Regs()[regs::PC] = clearThumb(addr);
}

ARMContext::ARMContext() {
    m_Env = std::make_unique<ARMEnvironment>(makeDefaultMem(), STACK_SIZE);
    m_ExMon = std::make_unique<dynarmic::ExclusiveMonitor>(1);
}

Expected<void> ARMContext::loadBinary(const std::span<const u8> buffer) {
    Binary binary(buffer);
    auto env = environment();
    auto mem = env->memoryManager();

    // Allocate and map segments.
    std::vector<std::pair<uaddr, usize>> secBases;
    DASHLE_TRY_EXPECTED_CONST(loadSegments, binary.segments(binary::elf::PT_LOAD));
    // TODO: ensure this is page aligned.
    DASHLE_TRY_EXPECTED_CONST(binaryBase, mem->findFreeAddr(0u));

    for (const auto segment : loadSegments) {
        DASHLE_TRY_EXPECTED_CONST(allocBase, binary.segmentAllocBase(segment));
        DASHLE_TRY_EXPECTED_CONST(allocSize, binary.segmentAllocSize(segment));
        DASHLE_TRY_EXPECTED_CONST(block, mem->allocate(binaryBase + allocBase, allocSize, host::memory::flags::PERM_READ_WRITE | host::memory::flags::FORCE_HINT));

        // Host offset is the memory offset for the first byte, relative to the allocation base.
        DASHLE_ASSERT(segment->p_filesz < block->size);
        std::copy(buffer.data() + segment->p_offset,
            buffer.data() + segment->p_offset + segment->p_filesz,
            reinterpret_cast<u8*>(block->hostBase) + (segment->p_vaddr - allocBase));

        secBases.push_back({ block->virtualBase, binary.wrapPermissionFlags(segment->p_flags) });
    }

    // Handle relocations.
    const auto& relocs = binary.relocs();
    DASHLE_TRY_EXPECTED_VOID(env->allocateILT(relocs.size()));

    for (const auto& reloc : binary.relocs()) {
        DASHLE_TRY_EXPECTED_CONST(patchAddr, env->virtualToHost(binaryBase + reloc.patchOffset));

        if (reloc.kind == binary::RelocKind::Relative) {
            if (reloc.addend)
                *reinterpret_cast<ELFConfig::AddrType*>(patchAddr) = binaryBase + reloc.addend;
            else
                *reinterpret_cast<ELFConfig::AddrType*>(patchAddr) += binaryBase;
            continue;
        }

        if (reloc.kind == binary::RelocKind::Symbol) {
            DASHLE_ASSERT_WRAPPER_CONST(symbolName, reloc.symbolName);
            // We ignore the addend.
            *reinterpret_cast<ELFConfig::AddrType*>(patchAddr) = env->insertILTEntry(symbolName);
            continue;
        }

        DASHLE_UNREACHABLE("Invalid relocation kind!");
    }

    // Set memory permissions.
    for (const auto [vbase, flags] : secBases) {
        DASHLE_TRY_EXPECTED_VOID(mem->setFlags(vbase, flags));
    }

    // Get initializers and finalizers.
    const auto initWrapper = binary.initArrayInfo();
    if (initWrapper) {
        const auto& initArrayInfo = initWrapper.value();
        DASHLE_TRY_EXPECTED_CONST(hostAddr, env->virtualToHost(binaryBase + initArrayInfo.offset));
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
        DASHLE_TRY_EXPECTED_CONST(hostAddr, env->virtualToHost(binaryBase + finiArrayInfo.offset));
        const auto finiArray = reinterpret_cast<const ELFConfig::AddrType*>(hostAddr);
        m_Finalizers.clear();
        for (auto i = 0u; i < finiArrayInfo.size; ++i) {
            const auto addr = finiArray[i];
            if (addr != 0 && addr != -1)
                m_Finalizers.push_back(finiArray[i]);
        }
    }

    initJit();
    return EXPECTED_VOID;
}

void ARMContext::reset() {
    DASHLE_ASSERT(m_Jit);

    // Run finalizers.
    for (auto vaddr : m_Finalizers) {
        // TODO: erroring.
        execute(vaddr);
    }

    initJit();
}

dynarmic::HaltReason ARMContext::execute(uaddr addr) {
    DASHLE_ASSERT(m_Jit);
    const auto stopAddr = invalidAddr();

    if (addr == stopAddr)
        return m_Jit->Run();
    
    m_Jit->Regs()[regs::LR] = clearThumb(stopAddr);
    setPC(addr);

    auto reason = EXEC_SUCCESS;
    while (reason == EXEC_SUCCESS) {
        reason = m_Jit->Run();
        if (m_Jit->Regs()[regs::PC] == clearThumb(stopAddr))
            break;
    }

    return reason;
}

dynarmic::HaltReason ARMContext::step(uaddr addr) {
    DASHLE_ASSERT(m_Jit);
    const auto stopAddr = invalidAddr();

    if (addr != stopAddr)
        setPC(addr);

    return m_Jit->Step();
}

void ARMContext::setRegister(usize id, u64 value) {
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

u64 ARMContext::getRegister(usize id) const {
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

void ARMContext::dump() const {
    if constexpr(DEBUG_MODE) {
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
}