#include "DasHLE/Binary/ELF.h"
#include "DasHLE/Guest/ARM/Context.h"

using namespace dashle;
using namespace dashle::guest;

constexpr static auto ARM_MEM_SIZE = static_cast<usize>(1u) << 32; // 4GB

static std::unique_ptr<host::memory::HostAllocator> makeDefaultAlloc() {
    auto alloc = std::make_unique<host::memory::HostAllocator>();
    DASHLE_ASSERT(alloc);
    return alloc;
}

static std::unique_ptr<arm::ARMContext> makeARMContext() {
    auto ctx = std::make_unique<arm::ARMContext>(
        std::make_unique<host::memory::MemoryManager>(makeDefaultAlloc(), ARM_MEM_SIZE));
    DASHLE_ASSERT(ctx);
    return ctx;
}

// GuestContext

Expected<std::unique_ptr<GuestContext>> dashle::guest::createContext(const host::fs::path& path) {
    std::vector<u8> buffer;
    return host::fs::readFile(path, buffer).and_then([&buffer](){
        return createContext(buffer);
    });
}

Expected<std::unique_ptr<GuestContext>> dashle::guest::createContext(const std::span<const u8> buffer) {
    std::unique_ptr<GuestContext> ctx;

    DASHLE_TRY_OPTIONAL_CONST(arch, binary::elf::detectArch(buffer), Error::InvalidArch);
    switch (arch) {
        case binary::elf::constants::EM_ARM:
            ctx = makeARMContext();
            break;
        default:
            return Unexpected(Error::InvalidArch);
    }

    DASHLE_TRY_EXPECTED_VOID(ctx->loadBinary(buffer));
    return ctx;
}