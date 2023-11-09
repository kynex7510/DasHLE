#include "DasHLE/Utils/ELF.h"
#include "DasHLE/Emu/ARM/Environment.h"

using namespace dashle;
using namespace dashle::utils;
using namespace dashle::emu::arm;

struct ConfigARM : elf::Config32, elf::ConfigLE {
    constexpr static auto ARCH = elf::EM_ARM;
};

ELF_ASSERT_CONFIG(ConfigARM);

// Environment

Expected<uaddr> Environment::virtualToHost(uaddr vaddr) const {
    DASHLE_ASSERT(m_Mem);
    return m_Mem->blockFromVAddr(vaddr).and_then([vaddr](const host::memory::AllocatedBlock* block) {
        return host::memory::virtualToHost(*block, vaddr);
    });
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
    const auto hostAddr = virtualToHost(vaddr);
    DASHLE_ASSERT(hostAddr);
    return *reinterpret_cast<const std::uint8_t*>(hostAddr.value());
}

std::uint16_t Environment::MemoryRead16(dynarmic32::VAddr vaddr) {
    const auto hostAddr = virtualToHost(vaddr);
    DASHLE_ASSERT(hostAddr);
    return *reinterpret_cast<const std::uint16_t*>(hostAddr.value());
}

std::uint32_t Environment::MemoryRead32(dynarmic32::VAddr vaddr) {
    const auto hostAddr = virtualToHost(vaddr);
    DASHLE_ASSERT(hostAddr);
    return *reinterpret_cast<const std::uint32_t*>(hostAddr.value());
}

std::uint64_t Environment::MemoryRead64(dynarmic32::VAddr vaddr) {
    const auto hostAddr = virtualToHost(vaddr);
    DASHLE_ASSERT(hostAddr);
    return *reinterpret_cast<const std::uint64_t*>(hostAddr.value());
}

void Environment::MemoryWrite8(dynarmic32::VAddr vaddr, std::uint8_t value) {
    const auto hostAddr = virtualToHost(vaddr);
    DASHLE_ASSERT(hostAddr);
    *reinterpret_cast<std::uint8_t*>(hostAddr.value()) = value;
}

void Environment::MemoryWrite16(dynarmic32::VAddr vaddr, std::uint16_t value) {
    const auto hostAddr = virtualToHost(vaddr);
    DASHLE_ASSERT(hostAddr);
    *reinterpret_cast<std::uint16_t*>(hostAddr.value()) = value;
}

void Environment::MemoryWrite32(dynarmic32::VAddr vaddr, std::uint32_t value) {
    const auto hostAddr = virtualToHost(vaddr);
    DASHLE_ASSERT(hostAddr);
    *reinterpret_cast<std::uint32_t*>(hostAddr.value()) = value;
}

void Environment::MemoryWrite64(dynarmic32::VAddr vaddr, std::uint64_t value) {
    const auto hostAddr = virtualToHost(vaddr);
    DASHLE_ASSERT(hostAddr);
    *reinterpret_cast<std::uint64_t*>(hostAddr.value()) = value;
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

Environment::Environment(std::unique_ptr<host::memory::MemoryManager> mem, usize stackSize) : m_Mem(std::move(mem)) {
    // Allocate zero page.
    DASHLE_ASSERT(m_Mem->allocate(0u, 0x1000, 0));

    // Allocate stack.
    if (stackSize) {
        auto ret = m_Mem->allocate(m_Mem->maxMemory() - stackSize, stackSize, host::memory::flags::READ_WRITE);
        DASHLE_ASSERT(ret);
        m_StackBase = ret.value();
    }
}

Expected<void> Environment::openBinary(const fs::path& path) {
    std::vector<u8> buffer;
    return fs::readFile(path, buffer).and_then([this, &buffer](){
        return loadBinary(buffer);
    });
}

Expected<void> Environment::loadBinary(const std::span<const u8> buffer) {
    if (!elf::isArch(buffer, ConfigARM::ARCH))
        return Unexpected(Error::InvalidArch);

    auto header = elf::getHeader<ConfigARM>(buffer);
    if (!header)
        return Unexpected(Error::InvalidObject);

    // Calculate allocation space for load segments.
    auto loadSegments = elf::getSegments<ConfigARM>(header, elf::PT_LOAD);
    if (loadSegments.empty())
        return Unexpected(Error::NoSegments);

    //
    DASHLE_LOG_LINE("Number of LOAD segments: {}", loadSegments.size());
    //

    const auto regionSize = elf::getSegmentsAllocSize<ConfigARM>(loadSegments);
    if (!regionSize)
        return Unexpected(Error::InvalidSegments);

    // Allocate memory and map segments.
    const auto hostBase = m_Mem->allocate(regionSize)
        .and_then([this](uaddr vbase) {
            m_BinaryBase = vbase;
            return virtualToHost(vbase);
        });

    if (!hostBase)
        return Unexpected(hostBase.error());

    for (const auto segment : loadSegments) {
        std::copy(buffer.data() + segment->p_offset,
            buffer.data() + segment->p_offset + segment->p_filesz,
            reinterpret_cast<u8*>(hostBase.value()) + segment->p_vaddr);
    }

    // Handle relocations.
    // TODO

    m_BinaryType = BinaryType::Armeabi_v7a;
    return EXPECTED_VOID;
}

// DefaultEnvironment

static std::unique_ptr<host::memory::MemoryManager> makeDefaultMem() {
    return std::make_unique<host::memory::MemoryManager>(
        std::make_unique<host::memory::HostAllocator>(),
        static_cast<usize>(1u) << 32);
}

DefaultEnvironment::DefaultEnvironment(usize stackSize) : Environment(makeDefaultMem(), stackSize) {}