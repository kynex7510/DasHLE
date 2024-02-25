#ifndef _DASHLE_GUEST_ELFVM_H
#define _DASHLE_GUEST_ELFVM_H

#include "DasHLE/Host/FS.h"
#include "DasHLE/Host/Memory.h"
#include "DasHLE/Host/Bridge.h"
#include "DasHLE/Guest/StackVM.h"
#include "DasHLE/Guest/ARM/ARM.h"
#include "DasHLE/Guest/AArch64/AArch64.h"
#include "DasHLE/Binary/ELF.h"

#include <type_traits>

namespace dashle::guest {

class ELFVM {
protected:
    using VM = Poly<StackVM
#if defined(DASHLE_HAS_GUEST_ARM)
        ,arm::ARMVM
#endif // DASHLE_HAS_GUEST_ARM
#if defined (DASHLE_HAS_GUEST_AARCH64)
        ,aarch64::AArch64VM
#endif // DASHLE_HAS_GUEST_AARCH64
    >;

    std::shared_ptr<host::memory::MemoryManager> m_Mem;
    std::shared_ptr<host::bridge::Bridge> m_Bridge;
    elf::ELF m_Elf;
    std::vector<uaddr> m_Initializers;
    std::vector<uaddr> m_Finalizers;
    VM m_VM;

    Expected<void> runInitializers();
    Expected<void> runFinalizers();

public:
    ELFVM(std::shared_ptr<host::memory::MemoryManager> mem, std::shared_ptr<host::bridge::Bridge> bridge);

    Expected<void> loadBinary(std::vector<u8>&& buffer);
    Expected<void> loadBinary(const host::fs::path& path);    
};

} // namespace dashle::guest

#endif /* _DASHLE_GUEST_ELFVM_H */