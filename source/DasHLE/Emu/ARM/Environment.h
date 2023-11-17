#ifndef _DASHLE_EMU_ARM_ENVIRONMENT_H
#define _DASHLE_EMU_ARM_ENVIRONMENT_H

#include "dynarmic/interface/A32/a32.h"

#include "DasHLE/Host/FS.h"
#include "DasHLE/Host/Memory.h"
#include "DasHLE/Emu/ARM/Relocs.h"

//
#include <unordered_map>
//

namespace dashle::emu::arm {

namespace dynarmic = Dynarmic;
namespace dynarmic32 = dynarmic::A32;

enum class BinaryVersion {
    Armeabi,    // v5TE
    Armeabi_v7a // v7
};

class Environment : public dynarmic32::UserCallbacks, public RelocationDelegate {
    std::unique_ptr<host::memory::MemoryManager> m_Mem;
    Optional<uaddr> m_BinaryBase;
    Optional<uaddr> m_StackBase;
    Optional<uaddr> m_StackTop;
    //
    uaddr m_ILTBase;
    usize m_ILTSize;
    std::unordered_map<uaddr, std::string> m_ILT;
    //
    Optional<BinaryVersion> m_BinaryVersion;
    std::vector<uaddr> m_Initializers;
    std::vector<uaddr> m_Finalizers;

    Expected<uaddr> virtualToHostChecked(uaddr vaddr, usize flags) const;

    /* Relocation callbacks */

    Expected<uaddr> virtualToHost(uaddr vaddr) const override;
    Expected<uaddr> resolveSymbol(const std::string& symbolName) override;

    /* Dynarmic callbacks */

    bool PreCodeReadHook(bool isThumb, dynarmic32::VAddr pc, dynarmic32::IREmitter& ir) override final;

    std::optional<std::uint32_t> MemoryReadCode(dynarmic32::VAddr vaddr) override final;

    std::uint8_t MemoryRead8(dynarmic32::VAddr vaddr) override final;
    std::uint16_t MemoryRead16(dynarmic32::VAddr vaddr) override final;
    std::uint32_t MemoryRead32(dynarmic32::VAddr vaddr) override final;
    std::uint64_t MemoryRead64(dynarmic32::VAddr vaddr) override final;

    void MemoryWrite8(dynarmic32::VAddr vaddr, std::uint8_t value) override final;
    void MemoryWrite16(dynarmic32::VAddr vaddr, std::uint16_t value) override final;
    void MemoryWrite32(dynarmic32::VAddr vaddr, std::uint32_t value) override final;
    void MemoryWrite64(dynarmic32::VAddr vaddr, std::uint64_t value) override final;

    bool MemoryWriteExclusive8(dynarmic32::VAddr vaddr, std::uint8_t value, std::uint8_t expected) override final {
        MemoryWrite8(vaddr, value);
        return true;
    }

    bool MemoryWriteExclusive16(dynarmic32::VAddr vaddr, std::uint16_t value, std::uint16_t expected) override final {
        MemoryWrite16(vaddr, value);
        return true;
    }

    bool MemoryWriteExclusive32(dynarmic32::VAddr vaddr, std::uint32_t value, std::uint32_t expected) override final {
        MemoryWrite32(vaddr, value);
        return true;
    }

    bool MemoryWriteExclusive64(dynarmic32::VAddr vaddr, std::uint64_t value, std::uint64_t expected) override final {
        MemoryWrite64(vaddr, value);
        return true;
    }

    bool IsReadOnlyMemory(dynarmic32::VAddr vaddr) override final;

    void InterpreterFallback(dynarmic32::VAddr pc, usize numInstructions) override final;
    void CallSVC(u32 swi) override final;
    void ExceptionRaised(dynarmic32::VAddr pc, dynarmic32::Exception exception) override final;

    void AddTicks(std::uint64_t ticks) override final {}
    std::uint64_t GetTicksRemaining() override final { return static_cast<u64>(-1); }

public:
    Environment(std::unique_ptr<host::memory::MemoryManager> mem, usize stackSize);
    virtual ~Environment() {}

    Expected<void> openBinary(const host::fs::path& path);
    Expected<void> loadBinary(const std::span<const u8> buffer);

    Optional<uaddr> binaryBase() const { return m_BinaryBase; }
    Optional<uaddr> stackBase() const { return m_StackBase; }
    Optional<uaddr> stackTop() const { return m_StackTop; }
    Optional<BinaryVersion> binaryVersion() const { return m_BinaryVersion; }
    const std::vector<uaddr>& initializers() { return m_Initializers; }
    const std::vector<uaddr>& finalizers() { return m_Finalizers; }
};

class DefaultEnvironment final : public Environment {
public:
    DefaultEnvironment(usize stackSize);
};

} // namespace dashle::emu::arm

#endif /* _DASHLE_ARM_ENVIRONMENT_H */