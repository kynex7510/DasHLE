#include "DasHLE/Support/Math.h"
#include "DasHLE/Guest/ELFVM.h"

using namespace dashle;
namespace regs = dashle::guest::arm::regs;

constexpr const char BINARY_PATH[] = "/home/user/Desktop/GD/libgame.so";

constexpr static usize PAGE_SIZE = 0x1000; // 4KB
static_assert(dashle::isPowerOfTwo(PAGE_SIZE));

constexpr static usize STACK_SIZE = 1024 * 1024; // 1MB
static_assert(dashle::alignDown<usize>(STACK_SIZE, PAGE_SIZE) == STACK_SIZE);

constexpr usize MEM_4GB = static_cast<usize>(1u) << 32;

// MyVM

class MyVM final : public guest::ELFVM {
    Expected<void> populateBridge() override {
        return EXPECTED_VOID;
    }

public:
    MyVM(std::shared_ptr<host::memory::MemoryManager> mem) : ELFVM(mem, PAGE_SIZE, STACK_SIZE) {}
};

template <typename T>
static void successOrDie(Expected<T>&& result) {
    if (!result.has_value()) {
        DASHLE_UNREACHABLE("ERROR: {}", result.error());
    }
}

int main() {
    // Initialize memory.
    auto mem = std::make_shared<host::memory::MemoryManager>(
        std::make_unique<host::memory::HostAllocator>(),
        MEM_4GB);

    // Create VM.
    MyVM vm(mem);

    successOrDie(vm.loadBinary(BINARY_PATH));
    successOrDie(vm.runInitializers());
    return 0;
}