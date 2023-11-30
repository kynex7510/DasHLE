#include "dynarmic/frontend/A32/a32_ir_emitter.h"

#include "DasHLE/Guest/ARM/Environment.h"

using namespace dashle;
using namespace dashle::guest::arm;

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

Expected<uaddr> ARMEnvironment::virtualToHost(uaddr vaddr) const {
    DASHLE_ASSERT(m_Mem);
    return m_Mem->blockFromVAddr(vaddr).and_then([vaddr](const host::memory::AllocatedBlock* block) {
        return host::memory::virtualToHost(*block, vaddr);
    });
}

bool ARMEnvironment::PreCodeReadHook(bool isThumb, dynarmic32::VAddr pc, dynarmic32::IREmitter& ir) {
    return true;
}

std::optional<std::uint32_t> ARMEnvironment::ARMEnvironment::MemoryReadCode(dynarmic32::VAddr vaddr) {
    const auto hostAddr = virtualToHostChecked(vaddr, host::memory::flags::PERM_EXEC);
    if (hostAddr)
        return *reinterpret_cast<const std::uint32_t*>(hostAddr.value());

    return {};
}

std::uint8_t ARMEnvironment::ARMEnvironment::MemoryRead8(dynarmic32::VAddr vaddr) {
    const auto addr = virtualToHostChecked(vaddr, host::memory::flags::PERM_READ);
    DASHLE_ASSERT(addr);
    return *reinterpret_cast<const std::uint8_t*>(addr.value());
}

std::uint16_t ARMEnvironment::ARMEnvironment::MemoryRead16(dynarmic32::VAddr vaddr) {
    const auto addr = virtualToHostChecked(vaddr, host::memory::flags::PERM_READ);
    DASHLE_ASSERT(addr);
    return *reinterpret_cast<const std::uint16_t*>(addr.value());
}

std::uint32_t ARMEnvironment::ARMEnvironment::MemoryRead32(dynarmic32::VAddr vaddr) {
    const auto addr = virtualToHostChecked(vaddr, host::memory::flags::PERM_READ);
        DASHLE_ASSERT(addr);
        return *reinterpret_cast<const std::uint32_t*>(addr.value());
}

std::uint64_t ARMEnvironment::ARMEnvironment::MemoryRead64(dynarmic32::VAddr vaddr) {
        const auto addr = virtualToHostChecked(vaddr, host::memory::flags::PERM_READ);
        DASHLE_ASSERT(addr);
        return *reinterpret_cast<const std::uint64_t*>(addr.value());
    }

void ARMEnvironment::MemoryWrite8(dynarmic32::VAddr vaddr, std::uint8_t value) {
    const auto addr = virtualToHostChecked(vaddr, host::memory::flags::PERM_WRITE);
    DASHLE_ASSERT(addr);
    *reinterpret_cast<std::uint8_t*>(addr.value()) = value;
}

void ARMEnvironment::MemoryWrite16(dynarmic32::VAddr vaddr, std::uint16_t value) {
    const auto addr = virtualToHostChecked(vaddr, host::memory::flags::PERM_WRITE);
    DASHLE_ASSERT(addr);
    *reinterpret_cast<std::uint16_t*>(addr.value()) = value;
}

void ARMEnvironment::MemoryWrite32(dynarmic32::VAddr vaddr, std::uint32_t value)  {
    const auto addr = virtualToHostChecked(vaddr, host::memory::flags::PERM_WRITE);
    DASHLE_ASSERT(addr);
    *reinterpret_cast<std::uint32_t*>(addr.value()) = value;
}
    
void ARMEnvironment::MemoryWrite64(dynarmic32::VAddr vaddr, std::uint64_t value) {
    const auto addr = virtualToHostChecked(vaddr, host::memory::flags::PERM_WRITE);
    DASHLE_ASSERT(addr);
    *reinterpret_cast<std::uint64_t*>(addr.value()) = value;
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
            
    return true;
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