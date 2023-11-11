#ifndef _DASHLE_EMU_ARM_ENVIRONMENT_H
#define _DASHLE_EMU_ARM_ENVIRONMENT_H

#include "dynarmic/interface/A32/a32.h"

#include "DasHLE/Utils/FS.h"
#include "DasHLE/Host/Memory.h"
#include "DasHLE/Emu/ARM/Relocs.h"

namespace dashle::emu::arm {

namespace dynarmic = Dynarmic;
namespace dynarmic32 = dynarmic::A32;
namespace fs = dashle::utils::fs;

enum class BinaryVersion {
    Armeabi,    // v5TE
    Armeabi_v7a // v7
};

class Environment : public dynarmic32::UserCallbacks, public RelocationDelegate {
    std::unique_ptr<host::memory::MemoryManager> m_Mem;
    Optional<uaddr> m_BinaryBase;
    Optional<uaddr> m_StackBase;
    usize m_StackSize;
    Optional<BinaryVersion> m_BinaryVersion;

    /* Dynarmic callbacks */

    void PreCodeTranslationHook(bool isThumb, dynarmic32::VAddr pc, dynarmic32::IREmitter& ir) override final;

    std::optional<std::uint32_t> MemoryReadCode(dynarmic32::VAddr vaddr) override final;

    std::uint8_t MemoryRead8(dynarmic32::VAddr vaddr) override final;
    std::uint16_t MemoryRead16(dynarmic32::VAddr vaddr) override final;
    std::uint32_t MemoryRead32(dynarmic32::VAddr vaddr) override final;
    std::uint64_t MemoryRead64(dynarmic32::VAddr vaddr) override final;

    void MemoryWrite8(dynarmic32::VAddr vaddr, std::uint8_t value) override final;
    void MemoryWrite16(dynarmic32::VAddr vaddr, std::uint16_t value) override final;
    void MemoryWrite32(dynarmic32::VAddr vaddr, std::uint32_t value) override final;
    void MemoryWrite64(dynarmic32::VAddr vaddr, std::uint64_t value) override final;

    bool IsReadOnlyMemory(dynarmic32::VAddr vaddr) override final;

    void InterpreterFallback(dynarmic32::VAddr pc, usize numInstructions) override final;
    void CallSVC(u32 swi) override final;
    void ExceptionRaised(dynarmic32::VAddr pc, dynarmic32::Exception exception) override final;

    void AddTicks(std::uint64_t ticks) override final {}
    std::uint64_t GetTicksRemaining() override final { return static_cast<u64>(-1); }

    /* Relocation callbacks */

    Expected<uaddr> virtualToHost(uaddr vaddr) const override;
    Expected<uaddr> resolveSymbol(const std::string& symbolName) override;

public:
    Environment(std::unique_ptr<host::memory::MemoryManager> mem, usize stackSize);
    virtual ~Environment() {}

    Expected<void> openBinary(const fs::path& path);
    Expected<void> loadBinary(const std::span<const u8> buffer);

    Optional<uaddr> binaryBase() const { return m_BinaryBase; }
    Optional<uaddr> stackBase() const { return m_StackBase; }
    usize stackSize() const { return m_StackSize; }
    Optional<BinaryVersion> binaryVersion() const { return m_BinaryVersion; }
};

class DefaultEnvironment final : public Environment {
public:
    DefaultEnvironment(usize stackSize);
};

} // namespace dashle::emu::arm

#endif /* _DASHLE_ARM_ENVIRONMENT_H */