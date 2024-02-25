#ifndef _DASHLE_GUEST_STACKVM_H
#define _DASHLE_GUEST_STACKVM_H

#include "DasHLE/Dynarmic.h"
#include "DasHLE/Host/Memory.h"

#include <type_traits>

namespace dashle::guest {

constexpr static auto VM_EXEC_SUCCESS = static_cast<dynarmic::HaltReason>(0u);

class StackVM {
protected:
    std::shared_ptr<host::memory::MemoryManager> m_Mem;
    uaddr m_StackBase = 0u;
    uaddr m_StackTop = 0u;

    StackVM(std::shared_ptr<host::memory::MemoryManager> mem, usize stackSize, usize alignment = 0u);

public:
    virtual ~StackVM();

    virtual dynarmic::HaltReason execute(Optional<uaddr> addr = {}) = 0;
    virtual dynarmic::HaltReason step(Optional<uaddr> addr = {}) = 0;

    virtual void clearCache() = 0;
    virtual void invalidateCache(uaddr addr, usize size) = 0;

    virtual void setRegister(usize id, u64 value) = 0;
    virtual u64 getRegister(usize id) const = 0;

    virtual void dump() const {}
};

} // namespace dashle::guest

#endif /* _DASHLE_GUEST_VM_H */