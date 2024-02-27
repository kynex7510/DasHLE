#ifndef _DASHLE_GUEST_ELFVM_H
#define _DASHLE_GUEST_ELFVM_H

#include "DasHLE/Binary/ELF.h"
#include "DasHLE/Host/FS.h"
#include "DasHLE/Host/Memory.h"
#include "DasHLE/Host/Bridge.h"
#include "DasHLE/Guest/VM.h"
#include "DasHLE/Guest/ARM/ARM.h"

#include <type_traits>

namespace dashle::guest {

class ELFVM {
protected:
    std::shared_ptr<host::memory::MemoryManager> m_Mem;
    std::shared_ptr<host::bridge::Bridge> m_Bridge;
    std::unique_ptr<VM> m_VM;
    const usize m_PageSize = 0u;
    uaddr m_StackBase = 0u;
    uaddr m_StackTop = 0u;
    binary::elf::ELF m_Elf;
    std::vector<uaddr> m_LoadedSegments;
    std::vector<uaddr> m_Initializers;
    std::vector<uaddr> m_Finalizers;

    virtual Expected<void> populateBridge() = 0;

public:
    ELFVM(std::shared_ptr<host::memory::MemoryManager> mem, usize pageSize, usize stackSize);
    virtual ~ELFVM();

    const binary::elf::ELF& elf() const { return m_Elf; }

    Expected<void> loadBinary(std::vector<u8>&& buffer);
    Expected<void> loadBinary(const host::fs::path& path);

    Expected<void> runInitializers();
    Expected<void> runFinalizers();
};

} // namespace dashle::guest

#endif /* _DASHLE_GUEST_ELFVM_H */