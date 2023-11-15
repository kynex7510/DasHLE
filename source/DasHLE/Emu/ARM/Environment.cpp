#include "DasHLE/Emu/ARM/Environment.h"

#include "dynarmic/frontend/A32/a32_ir_emitter.h"
#include "dynarmic/frontend/A32/a32_types.h"

using namespace dashle;
using namespace dashle::emu::arm;

constexpr static usize PAGE_SIZE = 0x1000;

// Environment

Expected<uaddr> Environment::virtualToHostChecked(uaddr vaddr, usize flags) const {
    if constexpr(DEBUG_MODE) {
        if (!(flags & host::memory::flags::PERM_EXEC))
            //DASHLE_LOG_LINE("Accessing addr: 0x{:X}", vaddr);
        flags &= host::memory::flags::PERM_MASK;
        const auto block = m_Mem->blockFromVAddr(vaddr);
        DASHLE_ASSERT(block);
        DASHLE_ASSERT(block.value()->flags & flags);
    }

    return virtualToHost(vaddr);
}

Expected<uaddr> Environment::virtualToHost(uaddr vaddr) const {
    DASHLE_ASSERT(m_Mem);
    return m_Mem->blockFromVAddr(vaddr).and_then([vaddr](const host::memory::AllocatedBlock* block) {
        return host::memory::virtualToHost(*block, vaddr);
    });
}

Expected<uaddr> Environment::resolveSymbol(const std::string& symbolName) {
    if (!m_ILTSize) {
        DASHLE_TRY_EXPECTED_CONST(addr, m_Mem->findFreeAddr(0u));
        m_ILTBase = addr;
    }

    const auto symValue = m_ILTBase + m_ILTSize;
    DASHLE_LOG_LINE("Resolved symbol {} = 0x{:X}", symbolName, symValue);
    m_ILT.insert({ symValue, symbolName });
    m_ILTSize += 4;
    return symValue;
}

bool Environment::PreCodeReadHook(bool isThumb, dynarmic32::VAddr pc, dynarmic32::IREmitter& ir) {
    if (auto it = m_ILT.find(pc); it != m_ILT.end()) {
        DASHLE_LOG_LINE("Attempted to call import \"{}\" (vaddr=0x{:X})", it->second, it->first);

        const auto lr = ir.GetRegister(dynarmic32::Reg::LR);
        ir.BXWritePC(lr);
        //ir.current_location.AdvancePC(isThumb ? 2 : 4);
        ir.SetTerm(dynarmic::IR::Term::ReturnToDispatch{});
        return true;
    }

    return false;
}

std::optional<std::uint32_t> Environment::MemoryReadCode(dynarmic32::VAddr vaddr) {
    //DASHLE_LOG_LINE("Exec @ 0x{:X}", vaddr);
    const auto hostAddr = virtualToHostChecked(vaddr, host::memory::flags::PERM_EXEC);
    if (hostAddr)
        return *reinterpret_cast<const std::uint32_t*>(hostAddr.value());

    return {};
}

std::uint8_t Environment::MemoryRead8(dynarmic32::VAddr vaddr) {
    const auto addr = virtualToHostChecked(vaddr, host::memory::flags::PERM_READ);
    DASHLE_ASSERT(addr);
    return *reinterpret_cast<const std::uint8_t*>(addr.value());
}

std::uint16_t Environment::MemoryRead16(dynarmic32::VAddr vaddr) {
    const auto addr = virtualToHostChecked(vaddr, host::memory::flags::PERM_READ);
    DASHLE_ASSERT(addr);
    return *reinterpret_cast<const std::uint16_t*>(addr.value());
}

std::uint32_t Environment::MemoryRead32(dynarmic32::VAddr vaddr) {
    const auto addr = virtualToHostChecked(vaddr, host::memory::flags::PERM_READ);
    DASHLE_ASSERT(addr);
    return *reinterpret_cast<const std::uint32_t*>(addr.value());
}

std::uint64_t Environment::MemoryRead64(dynarmic32::VAddr vaddr) {
    const auto addr = virtualToHostChecked(vaddr, host::memory::flags::PERM_READ);
    DASHLE_ASSERT(addr);
    return *reinterpret_cast<const std::uint64_t*>(addr.value());
}

void Environment::MemoryWrite8(dynarmic32::VAddr vaddr, std::uint8_t value) {
    const auto addr = virtualToHostChecked(vaddr, host::memory::flags::PERM_WRITE);
    DASHLE_ASSERT(addr);
    *reinterpret_cast<std::uint8_t*>(addr.value()) = value;
}

void Environment::MemoryWrite16(dynarmic32::VAddr vaddr, std::uint16_t value) {
    const auto addr = virtualToHostChecked(vaddr, host::memory::flags::PERM_WRITE);
    DASHLE_ASSERT(addr);
    *reinterpret_cast<std::uint16_t*>(addr.value()) = value;
}

void Environment::MemoryWrite32(dynarmic32::VAddr vaddr, std::uint32_t value) {
    const auto addr = virtualToHostChecked(vaddr, host::memory::flags::PERM_WRITE);
    DASHLE_ASSERT(addr);
    *reinterpret_cast<std::uint32_t*>(addr.value()) = value;
}

void Environment::MemoryWrite64(dynarmic32::VAddr vaddr, std::uint64_t value) {
    const auto addr = virtualToHostChecked(vaddr, host::memory::flags::PERM_WRITE);
    DASHLE_ASSERT(addr);
    *reinterpret_cast<std::uint64_t*>(addr.value()) = value;
}

bool Environment::IsReadOnlyMemory(dynarmic32::VAddr vaddr) {
    DASHLE_ASSERT(m_Mem);
    if (auto block = m_Mem->blockFromVAddr(vaddr))
        return !(block.value()->flags & host::memory::flags::PERM_WRITE);

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
    DASHLE_UNREACHABLE("Unimplemented exception handling (pc=0x{:X}, exception={})", pc, static_cast<u32>(exception));
}

Environment::Environment(std::unique_ptr<host::memory::MemoryManager> mem, usize stackSize) : m_Mem(std::move(mem)) {
    // Allocate zero page.
    DASHLE_ASSERT(m_Mem->allocate(0u, PAGE_SIZE, 0));

    // Allocate stack.
    if (stackSize) {
        auto ret = m_Mem->allocate(m_Mem->maxMemory() - stackSize, stackSize, host::memory::flags::PERM_READ_WRITE);
        DASHLE_ASSERT(ret);
        m_StackBase = ret.value()->virtualBase;
        m_StackTop = m_StackBase.value() + stackSize;
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
        DASHLE_TRY_EXPECTED_CONST(block, m_Mem->allocate(allocBase, allocSize, host::memory::flags::PERM_READ_WRITE | host::memory::flags::FORCE_HINT));

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
    const auto initWrapper = elf::getInitArrayInfo(header);
    const auto finiWrapper = elf::getFiniArrayInfo(header);

    if (initWrapper) {
        const auto& initArrayInfo = initWrapper.value();
        DASHLE_TRY_EXPECTED_CONST(hostAddr, virtualToHost(m_BinaryBase.value() + initArrayInfo.offset));
        const auto initArray = reinterpret_cast<const elf::Config::AddrType*>(hostAddr);
        m_Initializers.clear();
        for (auto i = 0u; i < initArrayInfo.size; ++i) {
            const auto addr = initArray[i];
            if (addr != 0 && addr != -1)
                m_Initializers.push_back(addr);
        }
    }

    if (finiWrapper) {
        const auto& finiArrayInfo = finiWrapper.value();
        DASHLE_TRY_EXPECTED_CONST(hostAddr, virtualToHost(m_BinaryBase.value() + finiArrayInfo.offset));
        const auto finiArray = reinterpret_cast<const elf::Config::AddrType*>(hostAddr);
        m_Finalizers.clear();
        for (auto i = 0u; i < finiArrayInfo.size; ++i) {
            const auto addr = finiArray[i];
            if (addr != 0 && addr != -1)
                m_Finalizers.push_back(finiArray[i]);
        }
    }

    return EXPECTED_VOID;
}

// DefaultEnvironment

static std::unique_ptr<host::memory::MemoryManager> makeDefaultMem() {
    return std::make_unique<host::memory::MemoryManager>(
        std::make_unique<host::memory::HostAllocator>(),
        static_cast<usize>(1u) << 32);
}

DefaultEnvironment::DefaultEnvironment(usize stackSize) : Environment(makeDefaultMem(), stackSize) {}