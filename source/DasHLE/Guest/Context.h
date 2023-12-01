#ifndef _DASHLE_GUEST_CONTEXT_H
#define _DASHLE_GUEST_CONTEXT_H

#include "dynarmic/interface/halt_reason.h"
#include "DasHLE/Host/FS.h"

namespace dashle {
    namespace dynarmic = Dynarmic;
} // namespace dashle

namespace dashle::guest {

constexpr static auto EXEC_SUCCESS = static_cast<dynarmic::HaltReason>(0u);

struct GuestContext {
    virtual ~GuestContext() {}
    virtual Expected<void> loadBinary(const std::span<const u8> buffer) = 0;
    virtual void reset() = 0;
    virtual dynarmic::HaltReason execute() = 0;
    virtual dynarmic::HaltReason execute(uaddr addr) = 0;
    virtual dynarmic::HaltReason step() = 0;
    virtual dynarmic::HaltReason step(uaddr addr) = 0;
    virtual void clearCache() = 0;
    virtual void invalidateCache(uaddr addr, usize size) = 0;
    virtual void setRegister(usize id, u64 value) = 0;
    virtual u64 getRegister(usize id) const = 0;
    virtual void dump() const {}
};

Expected<std::unique_ptr<GuestContext>> createContext(const host::fs::path& path);
Expected<std::unique_ptr<GuestContext>> createContext(const std::span<const u8> buffer);

} // namespace dashle::guest

#endif /* _DASHLE_GUEST_CONTEXT_H */