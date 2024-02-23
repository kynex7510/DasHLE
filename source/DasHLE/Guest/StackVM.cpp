#include "DasHLE/Support/Math.h"
#include "DasHLE/Guest/StackVM.h"

using namespace dashle;
using namespace dashle::guest;

// StackVM

StackVM::StackVM(std::shared_ptr<host::memory::MemoryManager> mem, usize stackSize, usize alignment)
    : m_Mem(mem) {
    // Alignment has to be a power of two.
    DASHLE_ASSERT(dashle::isPowerOfTwo(alignment));
    // Stack size has to be aligned.
    DASHLE_ASSERT(dashle::alignDown(stackSize, alignment) == stackSize);

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