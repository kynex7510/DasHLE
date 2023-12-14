#ifndef _DASHLE_HOST_INTEROP_H
#define _DASHLE_HOST_INTEROP_H

#include "DasHLE/Dynarmic.h"
#include "DasHLE/Host/Memory.h"

#include <unordered_map>

namespace dashle::host::interop {

// TODO: make this structure generic.
class IREmitterWrapper {
    dynarmic32::IREmitter* m_Ptr;

public:
    IREmitterWrapper(dynarmic32::IREmitter* ptr) : m_Ptr(ptr) {}
    dynarmic32::IREmitter* operator->() { return m_Ptr; };
};

using EmitterCallback = void(*)(IREmitterWrapper);

class InteropHandler final {
    std::shared_ptr<host::memory::MemoryManager> m_Mem;
    const usize m_MaxEntries;
    uaddr m_TableBase = 0;
    std::unordered_map<uaddr, EmitterCallback> m_Emitters;

public:
    InteropHandler(std::shared_ptr<host::memory::MemoryManager> mem, usize maxEntries);
    ~InteropHandler();
    
    Expected<uaddr> registerEmitterCallback(EmitterCallback cb);
    Expected<void> invokeEmitterCallback(uaddr vaddr, IREmitterWrapper emitter);
};

struct SymResolver {
    virtual Expected<uaddr> resolve(const std::string& sym) const = 0;
};

} // namespace dashle::host::interop

#endif /* _DASHLE_HOST_INTEROP_H */