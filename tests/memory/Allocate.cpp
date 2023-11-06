#include "Memory.h"
#include "Test.h"

// Test the implementation of allocate().
DASHLE_TEST(Memory::Allocate) {
    memory::MemoryManager mem(std::make_unique<memory::HostAllocator>(), static_cast<u32>(-1));

    auto ret = mem.allocate(0u);
    if (ret) {
        TEST_FAILED("Allocate did not return an error with size = 0!");
    }

    if (ret.error() != Error::InvalidSize) {
        TEST_FAILED(std::format("Allocate returned the wrong error code with size = 0: {}", errorAsString(ret.error())));
    }

    const auto flags = rand();
    ret = mem.allocate(mem.maxMemory(), flags);
    if (!ret) {
        TEST_FAILED(std::format("Allocation failed: {}", errorAsString(ret.error())));
    }

    auto block = mem.blockFromVAddr(ret.value());
    if (!block) {
        TEST_FAILED(std::format("Could not find the allocated block: {}", errorAsString(block.error())));
    }

    if (block.value()->flags != (flags & memory::MemoryManager::FLAG_MASK)) {
        TEST_FAILED("Flags mismatch!");
    }

    ret = mem.allocate(1);
    if (ret) {
        TEST_FAILED("Allocation shall not succeed with no memory available!");
    }

    if (ret.error() != Error::NoVirtualMemory) {
        TEST_FAILED(std::format("Allocate returned the wrong error code when no virtual memory: {}", errorAsString(ret.error())));
    }

    TEST_PASSED();
}