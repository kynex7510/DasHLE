#include "DasHLE/Support/Math.h"
#include "DasHLE/Guest/ELFVM.h"

#define REGISTER_FUNC(sym) m_Bridge->registerFunction<dashle::BITS_32, sym##Emulated>(#sym)

using namespace dashle;
namespace regs = dashle::guest::arm::regs;

using namespace regs;

constexpr const char BINARY_PATH[] = "/home/user/Desktop/GD/libgame.so";

constexpr static usize PAGE_SIZE = 0x1000; // 4KB
static_assert(dashle::isPowerOfTwo(PAGE_SIZE));

constexpr static usize STACK_SIZE = 1024 * 1024; // 1MB
static_assert(dashle::alignDown<usize>(STACK_SIZE, PAGE_SIZE) == STACK_SIZE);

constexpr usize MEM_4GB = static_cast<usize>(1u) << 32;

template <typename T>
using ptr32 = u32;

// MyVM

class MyVM final : public guest::ELFVM {
    Expected<void> populateBridge() override;
public:
    MyVM(std::shared_ptr<host::memory::MemoryManager> mem) : ELFVM(mem, PAGE_SIZE, STACK_SIZE) {}
};

static host::memory::MemoryManager* g_SharedMem = nullptr;
static MyVM* g_SharedVM = nullptr;

static void* hostPtr(uaddr vaddr) {
    DASHLE_ASSERT_WRAPPER_CONST(block, g_SharedMem->blockFromVAddr(vaddr));
    DASHLE_ASSERT_WRAPPER_CONST(addr, host::memory::virtualToHost(*block, vaddr));
    return reinterpret_cast<void*>(addr);
}

// int __cxa_atexit(void (*func) (void *), void * arg, void * dso_handle);
int __cxa_atexitEmulated(ptr32<void(*)(void*)> func, ptr32<void*> arg, ptr32<void*> dso_handle) { return 0; }

// int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));
int pthread_onceEmulated(ptr32<pthread_once_t*> once_control, ptr32<void(*)(void)> init_routine) {
    DASHLE_LOG_LINE("pthread_once called");
    DASHLE_LOG_LINE("- once_control: 0x{:X}", once_control);
    DASHLE_LOG_LINE("- init_routine: 0x{:X}", init_routine);
    return 88;
}

// void *memcpy(void * destination, const void * source, size_t num);
ptr32<void*> memcpyEmulated(ptr32<void*> destination, ptr32<const void*> source, u32 num) {
    DASHLE_LOG_LINE("memcpy called");
    DASHLE_LOG_LINE("- destination: 0x{:X}", destination);
    DASHLE_LOG_LINE("- source: 0x{:X}", source);
    DASHLE_LOG_LINE("- num: 0x{:X}", num);
    std::memcpy(hostPtr(destination), hostPtr(source), num);
    return destination;
}

Expected<void> MyVM::populateBridge() {
        REGISTER_FUNC(__cxa_atexit);
        //REGISTER_FUNC(pthread_once);
        //REGISTER_FUNC(memcpy);
        return EXPECTED_VOID;
}

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

    g_SharedMem = mem.get();

    // Create VM.
    MyVM vm(mem);
    g_SharedVM = &vm;

    successOrDie(vm.loadBinary(BINARY_PATH));
    successOrDie(vm.runInitializers());
    DASHLE_LOG_LINE("DONE!");
    return 0;
}