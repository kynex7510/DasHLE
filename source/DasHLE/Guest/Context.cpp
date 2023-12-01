#include "DasHLE/Binary/ELF.h"
#include "DasHLE/Guest/ARM/Context.h"

using namespace dashle;

// Context

Expected<std::unique_ptr<dashle::guest::Context>> dashle::guest::createContext(const host::fs::path& path) {
    std::vector<u8> buffer;
    return host::fs::readFile(path, buffer).and_then([&buffer](){
        return createContext(buffer);
    });
}

Expected<std::unique_ptr<dashle::guest::Context>> dashle::guest::createContext(const std::span<const u8> buffer) {
    std::unique_ptr<Context> ctx;

    DASHLE_TRY_OPTIONAL_CONST(arch, binary::elf::detectArch(buffer), Error::InvalidArch);
    switch (arch) {
        case binary::elf::constants::EM_ARM:
            ctx = std::make_unique<arm::ARMContext>();
            break;
        default:
            return Unexpected(Error::InvalidArch);
    }

    DASHLE_TRY_EXPECTED_VOID(ctx->loadBinary(buffer));
    return ctx;
}