#include "DasHLE/Guest/StackVM.h"

using namespace dashle;
using namespace dashle::guest;

// StackVM

StackVM::StackVM(std::shared_ptr<host::memory::MemoryManager> mem, usize stackSize, usize alignment)
    : m_Mem(mem) {
    // Allocate stack.
    DASHLE_ASSERT_WRAPPER_CONST(block, mem->allocate({
        .size = stackSize,
        .alignment = alignment,
        .hint = mem->virtualOffset() + mem->maxMemory() - stackSize,
        .flags = host::memory::flags::PERM_READ_WRITE | host::memory::flags::FORCE_HINT
    }));
    m_StackBase = block->virtualBase;
    m_StackTop = m_StackBase + block->size;
}

StackVM::~StackVM() { DASHLE_ASSERT(m_Mem->free(m_StackBase)); }