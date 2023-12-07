#include "dynarmic/frontend/A32/a32_ir_emitter.h"
#include "DasHLE/Guest/ARM/Context.h"

using namespace dashle;
using namespace dashle::guest::arm;

namespace dynarmic_ir = Dynarmic::IR;

// ARMEnvironment

Expected<uaddr> ARMEnvironment::virtualToHostChecked(uaddr vaddr, usize flags) const {
    if constexpr(DEBUG_MODE) {
        flags &= host::memory::flags::PERM_MASK;
        const auto block = m_Mem->blockFromVAddr(vaddr);
        DASHLE_ASSERT(block);
        DASHLE_ASSERT(block.value()->flags & flags);
    }

    return virtualToHost(vaddr);
}

bool ARMEnvironment::PreCodeReadHook(bool isThumb, dynarmic32::VAddr pc, dynarmic32::IREmitter& ir) {
    if (auto it = m_ILTEntries.find(pc); it != m_ILTEntries.end()) {
        // TODO: codegen imports.
        ir.CallSupervisor(dynarmic_ir::U32(dynarmic_ir::Value(1u)));
        ir.BXWritePC(ir.GetRegister(regs::asEnum(regs::LR)));
        ir.SetTerm(dynarmic_ir::Term::ReturnToDispatch{});
        return false;
    }

    return true;
}

std::optional<std::uint32_t> ARMEnvironment::MemoryReadCode(dynarmic32::VAddr vaddr) {
    const auto hostAddr = virtualToHostChecked(vaddr, host::memory::flags::PERM_EXEC);
    if (hostAddr)
        return *reinterpret_cast<const std::uint32_t*>(hostAddr.value());

    return {};
}

std::uint8_t ARMEnvironment::MemoryRead8(dynarmic32::VAddr vaddr) {
    DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_READ));
    return *reinterpret_cast<const std::uint8_t*>(addr);
}

std::uint16_t ARMEnvironment::MemoryRead16(dynarmic32::VAddr vaddr) {
    DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_READ));
    return *reinterpret_cast<const std::uint16_t*>(addr);
}

std::uint32_t ARMEnvironment::MemoryRead32(dynarmic32::VAddr vaddr) {
    DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_READ));
    return *reinterpret_cast<const std::uint32_t*>(addr);
}

std::uint64_t ARMEnvironment::MemoryRead64(dynarmic32::VAddr vaddr) {
    DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_READ));
    return *reinterpret_cast<const std::uint64_t*>(addr);
}

void ARMEnvironment::MemoryWrite8(dynarmic32::VAddr vaddr, std::uint8_t value) {
    DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_WRITE));
    *reinterpret_cast<std::uint8_t*>(addr) = value;
}

void ARMEnvironment::MemoryWrite16(dynarmic32::VAddr vaddr, std::uint16_t value) {
    DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_WRITE));
    *reinterpret_cast<std::uint16_t*>(addr) = value;
}

void ARMEnvironment::MemoryWrite32(dynarmic32::VAddr vaddr, std::uint32_t value)  {
    DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_WRITE));
    *reinterpret_cast<std::uint32_t*>(addr) = value;
}
    
void ARMEnvironment::MemoryWrite64(dynarmic32::VAddr vaddr, std::uint64_t value) {
    DASHLE_ASSERT_WRAPPER_CONST(addr, virtualToHostChecked(vaddr, host::memory::flags::PERM_WRITE));
    *reinterpret_cast<std::uint64_t*>(addr) = value;
}

bool ARMEnvironment::MemoryWriteExclusive8(dynarmic32::VAddr vaddr, std::uint8_t value, std::uint8_t expected) {
    // TODO
    MemoryWrite8(vaddr, value);
    return true;
}

bool ARMEnvironment::MemoryWriteExclusive16(dynarmic32::VAddr vaddr, std::uint16_t value, std::uint16_t expected) {
    // TODO
    MemoryWrite16(vaddr, value);
    return true;
}

bool ARMEnvironment::MemoryWriteExclusive32(dynarmic32::VAddr vaddr, std::uint32_t value, std::uint32_t expected) {
    // TODO
    MemoryWrite32(vaddr, value);
    return true;
}

bool ARMEnvironment::MemoryWriteExclusive64(dynarmic32::VAddr vaddr, std::uint64_t value, std::uint64_t expected) {
    // TODO
    MemoryWrite64(vaddr, value);
    return true;
}

bool ARMEnvironment::IsReadOnlyMemory(dynarmic32::VAddr vaddr) {
    DASHLE_ASSERT(m_Mem);
    if (auto block = m_Mem->blockFromVAddr(vaddr))
        return !(block.value()->flags & host::memory::flags::PERM_WRITE);
            
    return false;
}

void ARMEnvironment::CallSVC(u32 swi) {
    const auto pc = m_Ctx->getRegister(regs::PC);
    auto it = m_ILTEntries.find(pc);
    DASHLE_ASSERT(it != m_ILTEntries.end());
    const auto symbolName = it->second;

    if (symbolName == "pthread_once") {
        // TODO
        const auto ctrl = m_Ctx->getRegister(regs::R0);
        const auto routine = m_Ctx->getRegister(regs::R1);
        const auto lr = m_Ctx->getRegister(regs::LR);
        //DASHLE_LOG_LINE("pthread_once called with ctrl = 0x{:X}", ctrl);
        m_Ctx->setRegister(regs::R0, 88);
        return;
    }

    if (symbolName == "pthread_key_create") {
        // TODO
        m_Ctx->setRegister(regs::R0, 88);
        return;
    }

    if (symbolName == "memcpy") {
        const auto destVAddr = m_Ctx->getRegister(regs::R0);
        DASHLE_ASSERT_WRAPPER_CONST(destAddr, virtualToHost(destVAddr));
        DASHLE_ASSERT_WRAPPER_CONST(srcAddr, virtualToHost(m_Ctx->getRegister(regs::R1)));
        const auto size = m_Ctx->getRegister(regs::R2);
        memcpy(reinterpret_cast<void*>(destAddr), reinterpret_cast<void*>(srcAddr), size);
        m_Ctx->setRegister(regs::R0, destVAddr);
        return;
    }

    if (symbolName == "memset") {
        const auto destVAddr = m_Ctx->getRegister(regs::R0);
        DASHLE_ASSERT_WRAPPER_CONST(destAddr, virtualToHost(destVAddr));
        const auto value = m_Ctx->getRegister(regs::R1);
        const auto size = m_Ctx->getRegister(regs::R2);
        memset(reinterpret_cast<void*>(destAddr), value, size);
        m_Ctx->setRegister(regs::R0, destVAddr);
        return;
    }

    if (symbolName == "strcmp") {
        DASHLE_ASSERT_WRAPPER_CONST(s1, virtualToHost(m_Ctx->getRegister(regs::R0)));
        DASHLE_ASSERT_WRAPPER_CONST(s2, virtualToHost(m_Ctx->getRegister(regs::R1)));
        m_Ctx->setRegister(regs::R0, strcmp(reinterpret_cast<char*>(s1), reinterpret_cast<char*>(s2)));
        return;
    }

    if (symbolName == "wctob") {
        m_Ctx->setRegister(regs::R0, std::wctob(m_Ctx->getRegister(regs::R0)));
        return;
    }

    if (symbolName == "btowc") {
        m_Ctx->setRegister(regs::R0, std::btowc(m_Ctx->getRegister(regs::R0)));
        return;
    }

    if (symbolName == "wctype") {
        DASHLE_ASSERT_WRAPPER_CONST(ptr, virtualToHost(m_Ctx->getRegister(regs::R0)));
        m_Ctx->setRegister(regs::R0, std::wctype(reinterpret_cast<char*>(ptr)));
        return;
    }

    if (symbolName == "__cxa_atexit") {
        // Do nothing.
        return;
    }

    if (symbolName == "strlen") {
        DASHLE_ASSERT_WRAPPER_CONST(ptr, virtualToHost(m_Ctx->getRegister(regs::R0)));
        m_Ctx->setRegister(regs::R0, std::strlen(reinterpret_cast<char*>(ptr)));
        return;
    }

    if (symbolName == "malloc") {
        const auto size = m_Ctx->getRegister(regs::R0);
        DASHLE_ASSERT_WRAPPER_CONST(block, m_Mem->allocate(size, host::memory::flags::PERM_READ_WRITE));
        m_Ctx->setRegister(regs::R0, block->virtualBase);
        return;
    }

    DASHLE_LOG_LINE("Unhandled: {} (0x{:X})", symbolName, pc);
}

ARMEnvironment::ARMEnvironment(std::unique_ptr<host::memory::MemoryManager> mem, usize stackSize) {
    DASHLE_ASSERT(mem);
    m_Mem = std::move(mem);

    // Allocate zero page.
    DASHLE_ASSERT(m_Mem->allocate(0u, PAGE_SIZE, host::memory::flags::FORCE_HINT));

    // Allocate stack.
    DASHLE_ASSERT_WRAPPER_CONST(stackBlock, m_Mem->allocate(m_Mem->maxMemory() - stackSize, stackSize,
        host::memory::flags::PERM_READ_WRITE | host::memory::flags::FORCE_HINT));
    m_StackBase = stackBlock->virtualBase;
    m_StackTop = m_StackBase + stackBlock->size;
}

Expected<uaddr> ARMEnvironment::virtualToHost(uaddr vaddr) const {
    DASHLE_ASSERT(m_Mem);
    return m_Mem->blockFromVAddr(vaddr).and_then([vaddr](const host::memory::AllocatedBlock* block) {
        return host::memory::virtualToHost(*block, vaddr);
    });
}

Expected<void> ARMEnvironment::allocateILT(usize numEntries) {
    // TODO: page prot?
    DASHLE_TRY_EXPECTED_CONST(iltBlock, m_Mem->allocate(numEntries * sizeof(ELFConfig::AddrType), host::memory::flags::PERM_READ_WRITE));
    m_ILTBase = iltBlock->virtualBase;
    m_ILTNumEntries = numEntries;
    return EXPECTED_VOID;
}

uaddr ARMEnvironment::insertILTEntry(const std::string& symbolName) {
    DASHLE_ASSERT(m_ILTEntries.size() < m_ILTNumEntries);
    const auto vaddr = m_ILTBase + (m_ILTEntries.size() * sizeof(ELFConfig::AddrType));
    m_ILTEntries.insert({ vaddr, symbolName });
    return vaddr;
}