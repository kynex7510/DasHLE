#include "Memory.h"
#include "Test.h"

class DummyAllocator : public memory::HostAllocator {
    bool alloc(memory::AllocatedBlock& block) { return true; }
    void free(memory::AllocatedBlock& block) {}
};

// Make sure the memory manager is efficient in terms of space wasted.
DASHLE_TEST(Memory::OOM) {
    memory::MemoryManager mem(std::make_unique<DummyAllocator>(), static_cast<u32>(-1));
    usize numAllocs = 0;
    usize allocatedBytes = 0;
    while (true) {
        const auto size = randomSize<16, 32>();
        if (!mem.allocate(size))
            break;

        allocatedBytes += size;
        ++numAllocs;
    }

    const auto ratio = static_cast<float>(allocatedBytes) / mem.maxMemory() * 100;
    DASHLE_LOG(std::format("Num. of allocations: {}", numAllocs));
    DASHLE_LOG(std::format("Allocation ratio: {}% used memory", ratio));
    if (ratio <= 99.0f) {
        TEST_FAILED("The ratio is too low!");
    }

    TEST_PASSED();
}