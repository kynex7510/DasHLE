#ifndef _DASHLE_HOST_INTEROP_H
#define _DASHLE_HOST_INTEROP_H

#include "DasHLE/Dynarmic.h"
#include "DasHLE/Host/Memory.h"

#include <unordered_map>

namespace dashle::host::interop {

// TODO: make this structure generic.
class IREmitter {
    dynarmic32::IREmitter* m_Ptr;

public:
    IREmitter(dynarmic32::IREmitter* ptr) : m_Ptr(ptr) {}
    dynarmic32::IREmitter* operator->() { return m_Ptr; };
};

using Emitter = void(*)(IREmitter);

class InteropHandler final {
    std::shared_ptr<host::memory::MemoryManager> m_Mem;
    const usize m_MaxEntries;
    uaddr m_TableBase = 0;
    std::unordered_map<uaddr, Emitter> m_Emitters;

public:
    InteropHandler(std::shared_ptr<host::memory::MemoryManager> mem, usize maxEntries);
    ~InteropHandler();
    
    Expected<uaddr> registerEmitter(EmitterCallback cb);
    Expected<void> invokeEmitter(uaddr vaddr, IREmitterWrapper emitter);
};

} // namespace dashle::host::interop

#endif /* _DASHLE_HOST_INTEROP_H */