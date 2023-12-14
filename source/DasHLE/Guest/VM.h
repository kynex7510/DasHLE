#ifndef _DASHLE_GUEST_VM_H
#define _DASHLE_GUEST_VM_H

#include "DasHLE/Dynarmic.h"
#include "DasHLE/Host/FS.h"
#include "DasHLE/Host/Memory.h"
#include "DasHLE/Host/Interop.h"

namespace dashle::guest {

constexpr static auto EXEC_SUCCESS = static_cast<dynarmic::HaltReason>(0u);

struct GuestVM {
    virtual ~GuestVM() {}

    virtual Expected<void> loadBinary(const std::span<const u8> buffer) = 0;

    virtual Expected<uaddr> virtualToHost(uaddr vaddr) const = 0;
    virtual uaddr invalidAddr() const = 0;

    virtual dynarmic::HaltReason execute() = 0;
    virtual dynarmic::HaltReason execute(uaddr addr) = 0;
    virtual dynarmic::HaltReason step() = 0;
    virtual dynarmic::HaltReason step(uaddr addr) = 0;
    virtual void runInitializers() {}
    virtual void runFinalizers() {}

    virtual void clearCache() = 0;
    virtual void invalidateCache(uaddr addr, usize size) = 0;

    virtual void setRegister(usize id, u64 value) = 0;
    virtual u64 getRegister(usize id) const = 0;

    virtual void dump() const {}
};

struct VMArgs {
    std::shared_ptr<host::memory::MemoryManager> mem;
    std::shared_ptr<host::interop::InteropHandler> interop;
    const host::interop::SymResolver* resolver;
};

Expected<std::unique_ptr<GuestVM>> makeVMFromFile(const host::fs::path& path, const VMArgs& args);
Expected<std::unique_ptr<GuestVM>> makeVMFromBuffer(const std::span<const u8> buffer, const VMArgs& args);

} // namespace dashle::guest

#endif /* _DASHLE_GUEST_VM_H */