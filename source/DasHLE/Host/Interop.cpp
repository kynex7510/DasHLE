#include "DasHLE/Host/Interop.h"

using namespace dashle;
using namespace dashle::host::interop;

// Each entry is the size of an ARM instruction.
constexpr usize ENTRY_SIZE = 4;

// InteropHandler

InteropHandler::InteropHandler(std::shared_ptr<host::memory::MemoryManager> mem, usize maxEntries)
    : m_Mem(mem), m_MaxEntries(maxEntries) {
    DASHLE_ASSERT(maxEntries);

    // TODO: Which prot to use?
    DASHLE_ASSERT_WRAPPER_CONST(block, m_Mem->allocate(m_MaxEntries * ENTRY_SIZE, host::memory::flags::PERM_READ_WRITE));
    m_TableBase = block->virtualBase;
}

InteropHandler::~InteropHandler() {
    if (m_TableBase) {
        DASHLE_ASSERT(m_Mem->free(m_TableBase));
    }
}

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