#include "DasHLE.h"

#include <format>

using namespace dashle;
using MyAllocator = memory::GenericAllocator32;

int main() {
    usize allocatedBytes = 0;
    MyAllocator allocator;

    srand(time(nullptr));

    while (true) {
        const auto size = rand() % (1 << 24);
        if (!allocator.allocate(size))
            break;

        allocatedBytes += size;
    }

    const auto ratio = static_cast<double>(allocatedBytes) / allocator.maxMemory() * 100;
    DASHLE_LOG(std::format("Alloc ratio: {}\n", ratio));
    return ratio >= 99.8 ? 0 : 1;
}