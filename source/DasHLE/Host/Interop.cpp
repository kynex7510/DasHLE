#include "DasHLE/Host/Interop.h"

using namespace dashle;
using namespace dashle::host::interop;

// Each entry is the size of an ARM instruction.
constexpr static usize ENTRY_SIZE = 4;

// InteropHandler

InteropHandler::InteropHandler(std::shared_ptr<host::memory::MemoryManager> mem, usize maxEntries)
    : m_Mem(mem), m_MaxEntries(maxEntries) {
    DASHLE_ASSERT(maxEntries);

    DASHLE_ASSERT_WRAPPER_CONST(block, m_Mem->allocate({
        .size = m_MaxEntries * ENTRY_SIZE,
        .flags = 0u,
    }));
    m_TableBase = block->virtualBase;
}

InteropHandler::~InteropHandler() { DASHLE_ASSERT(m_Mem->free(m_TableBase)); }

Expected<uaddr> InteropHandler::registerEmitterCallback(EmitterCallback cb) {
    if (m_Emitters.size() >= m_MaxEntries)
        return Unexpected(Error::NoVirtualMemory);

    const auto vaddr = m_TableBase + (m_Emitters.size() * ENTRY_SIZE);
    m_Emitters[vaddr] = cb;
    return vaddr;
}

Expected<void> InteropHandler::invokeEmitterCallback(uaddr vaddr, IREmitterWrapper emitter) {
    if (auto it = m_Emitters.find(vaddr); it != m_Emitters.end()) {
        it->second(emitter);
        return EXPECTED_VOID;
    }

    return Unexpected(Error::NotFound);
}