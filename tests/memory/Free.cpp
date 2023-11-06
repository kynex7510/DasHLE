#include "Memory.h"
#include "Test.h"

// Test the implementation of free.
DASHLE_TEST(Memory::Free) {
    memory::MemoryManager mem(std::make_unique<memory::HostAllocator>(), static_cast<u32>(-1));

    while (true) {
        const auto size = randomSize<16, (1 << 16)>();
        auto ret = mem.allocate(size);
        if (!ret) {
            if (ret.error() == Error::NoVirtualMemory)
                break;

            TEST_FAILED(std::format("Allocate returned the wrong error code: %s", errorAsString(ret.error())));
        }

        if (rand() & 1) {
            if (auto freeRet = mem.free(ret.value()); !freeRet) {
                TEST_FAILED(std::format("Free operation failed: %s", errorAsString(freeRet.error())));
            }
        }
    }

    TEST_PASSED();
}