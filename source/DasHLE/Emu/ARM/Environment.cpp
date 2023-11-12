#include "DasHLE/Emu/ARM/Environment.h"

using namespace dashle;
using namespace dashle::emu::arm;

constexpr static usize PAGE_SIZE = 0x1000;

// Environment

uaddr Environment::virtualToHostForAccess(uaddr vaddr, usize flags) const {
    if constexpr(DEBUG_MODE) {
        flags &= host::memory::flags::READ_WRITE;
        const auto block = m_Mem->blockFromVAddr(vaddr);
        DASHLE_ASSERT(block);
        DASHLE_ASSERT(block.value()->flags & flags);
    }

    const auto hostAddr = virtualToHost(vaddr);
    DASHLE_ASSERT(hostAddr);
    return hostAddr.value();
}

Expected<uaddr> Environment::virtualToHost(uaddr vaddr) const {
    DASHLE_ASSERT(m_Mem);
    return m_Mem->blockFromVAddr(vaddr).and_then([vaddr](const host::memory::AllocatedBlock* block) {
        return host::memory::virtualToHost(*block, vaddr);
    });
}

Expected<uaddr> Environment::resolveSymbol(const std::string& symbolName) {
    // TODO
    return 0u;
}

void Environment::PreCodeTranslationHook(bool isThumb, dynarmic32::VAddr pc, dynarmic32::IREmitter& ir) {
    // TODO: handle imports.
    DASHLE_LOG_LINE("Code hook called (isThumb={}, pc=0x{:X})", isThumb, pc);
}

std::optional<std::uint32_t> Environment::MemoryReadCode(dynarmic32::VAddr vaddr) {
    const auto hostAddr = virtualToHost(vaddr);
    if (hostAddr)
        return *reinterpret_cast<const std::uint32_t*>(hostAddr.value());

    return {};
}

std::uint8_t Environment::MemoryRead8(dynarmic32::VAddr vaddr) {
    return *reinterpret_cast<const std::uint8_t*>(virtualToHostForAccess(vaddr, host::memory::flags::READ));
}

std::uint16_t Environment::MemoryRead16(dynarmic32::VAddr vaddr) {
    return *reinterpret_cast<const std::uint16_t*>(virtualToHostForAccess(vaddr, host::memory::flags::READ));
}

std::uint32_t Environment::MemoryRead32(dynarmic32::VAddr vaddr) {
    return *reinterpret_cast<const std::uint32_t*>(virtualToHostForAccess(vaddr, host::memory::flags::READ));
}

std::uint64_t Environment::MemoryRead64(dynarmic32::VAddr vaddr) {
    return *reinterpret_cast<const std::uint64_t*>(virtualToHostForAccess(vaddr, host::memory::flags::READ));
}

void Environment::MemoryWrite8(dynarmic32::VAddr vaddr, std::uint8_t value) {
    *reinterpret_cast<std::uint8_t*>(virtualToHostForAccess(vaddr, host::memory::flags::WRITE)) = value;
}

void Environment::MemoryWrite16(dynarmic32::VAddr vaddr, std::uint16_t value) {
    *reinterpret_cast<std::uint16_t*>(virtualToHostForAccess(vaddr, host::memory::flags::WRITE)) = value;
}

void Environment::MemoryWrite32(dynarmic32::VAddr vaddr, std::uint32_t value) {
    *reinterpret_cast<std::uint32_t*>(virtualToHostForAccess(vaddr, host::memory::flags::WRITE)) = value;
}

void Environment::MemoryWrite64(dynarmic32::VAddr vaddr, std::uint64_t value) {
    *reinterpret_cast<std::uint64_t*>(virtualToHostForAccess(vaddr, host::memory::flags::WRITE)) = value;
}

bool Environment::IsReadOnlyMemory(dynarmic32::VAddr vaddr) {
    DASHLE_ASSERT(m_Mem);
    if (auto block = m_Mem->blockFromVAddr(vaddr))
        return !(block.value()->flags & host::memory::flags::WRITE);

    return true;
}

void Environment::InterpreterFallback(dynarmic32::VAddr pc, usize numInstructions) {
    DASHLE_UNREACHABLE("Interpreter invoked (pc={}, numInstructions={})", pc, numInstructions);
}

void Environment::CallSVC(u32 swi) {
    // TODO: support SVC.
    DASHLE_UNREACHABLE("Unimplemented SVC call (swi={})", swi);
}

void Environment::ExceptionRaised(dynarmic32::VAddr pc, dynarmic32::Exception exception) {
    // TODO: handle exceptions.
    DASHLE_UNREACHABLE("Unimplemented exception handling (pc={}, exception={})", pc, static_cast<u32>(exception));
}

Environment::Environment(std::unique_ptr<host::memory::MemoryManager> mem, usize stackSize) : m_Mem(std::move(mem)), m_StackSize(stackSize) {
    // Allocate zero page.
    DASHLE_ASSERT(m_Mem->allocate(0u, PAGE_SIZE, 0));

    // Allocate stack.
    if (m_StackSize) {
        auto ret = m_Mem->allocate(m_Mem->maxMemory() - m_StackSize, m_StackSize, host::memory::flags::READ_WRITE);
        DASHLE_ASSERT(ret);
        m_StackBase = ret.value()->virtualBase;
    }
}

Expected<void> Environment::openBinary(const host::fs::path& path) {
    std::vector<u8> buffer;
    return host::fs::readFile(path, buffer).and_then([this, &buffer](){
        return loadBinary(buffer);
    });
}

Expected<void> Environment::loadBinary(const std::span<const u8> buffer) {
    // Get ELF header.
    DASHLE_TRY_EXPECTED_CONST(header, elf::getHeader(buffer));

    // Detect binary version.
    // TODO
    m_BinaryVersion = BinaryVersion::Armeabi_v7a;

    // Allocate and map segments.
    std::vector<std::pair<uaddr, usize>> secBases;
    DASHLE_TRY_EXPECTED_CONST(loadSegments, elf::getSegments(header, elf::PT_LOAD));
    DASHLE_TRY_EXPECTED(binaryBase, m_Mem->findFreeAddr(0u));

    for (const auto segment : loadSegments) {
        const auto allocBase = elf::getSegmentAllocBase(segment, binaryBase);
        DASHLE_TRY_EXPECTED_CONST(allocSize, elf::getSegmentAllocSize(segment));
        DASHLE_TRY_EXPECTED_CONST(block, m_Mem->allocate(allocBase, allocSize, host::memory::flags::READ_WRITE | host::memory::flags::FORCE_HINT));    
        
        const auto offset = segment->p_vaddr - (block->virtualBase - binaryBase);
        std::copy(buffer.data() + segment->p_offset,
            buffer.data() + segment->p_offset + segment->p_filesz,
            reinterpret_cast<u8*>(block->hostBase) + offset);

        secBases.push_back({ block->virtualBase, elf::wrapPermissionFlags(segment->p_flags) });
    }

    m_BinaryBase = binaryBase;

    // Handle relocations.
    DASHLE_TRY_EXPECTED_VOID(handleRelocations({
        .header = header,
        .virtualBase = m_BinaryBase.value(),
        .resolver = this
    }));

    // Set memory permissions.
    for (const auto [vbase, flags] : secBases) {
        DASHLE_TRY_EXPECTED_VOID(m_Mem->setFlags(vbase, flags));
    }

    // Get initializers and finalizers.
    DASHLE_TRY_EXPECTED_CONST(initializers, elf::getInitializers(header));
    DASHLE_TRY_EXPECTED_CONST(finalizers, elf::getFinalizers(header));
    m_Initializers = std::move(initializers);
    m_Finalizers = std::move(finalizers);

    return EXPECTED_VOID;
}

// DefaultEnvironment

static std::unique_ptr<host::memory::MemoryManager> makeDefaultMem() {
    return std::make_unique<host::memory::MemoryManager>(
        std::make_unique<host::memory::HostAllocator>(),
        static_cast<usize>(1u) << 32);
}

DefaultEnvironment::DefaultEnvironment(usize stackSize) : Environment(makeDefaultMem(), stackSize) {}