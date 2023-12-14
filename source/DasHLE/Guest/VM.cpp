#include "DasHLE/Binary/ELF.h"
#include "DasHLE/Guest/ARM.h"

using namespace dashle;
using namespace dashle::guest;

// GuestVM

Expected<std::unique_ptr<GuestVM>> dashle::guest::makeVMFromFile(const host::fs::path& path, const VMArgs& args) {
    std::vector<u8> buffer;
    return host::fs::readFile(path, buffer).and_then([&buffer, &args](){
        return makeVMFromBuffer(buffer, args);
    });
}

Expected<std::unique_ptr<GuestVM>> dashle::guest::makeVMFromBuffer(const std::span<const u8> buffer, const VMArgs& args) {
    std::unique_ptr<GuestVM> vm;

    if (!args.mem || !args.interop || !args.resolver)
        return Unexpected(Error::InvalidArgument);

    DASHLE_TRY_OPTIONAL_CONST(arch, binary::elf::detectArch(buffer), Error::InvalidArch);

    switch (arch) {
        case binary::elf::constants::EM_ARM:
            vm = std::make_unique<arm::VMImpl>(args.mem, args.interop, args.resolver);
            break;
        default:
            return Unexpected(Error::InvalidArch);
        }

    DASHLE_TRY_EXPECTED_VOID(vm->loadBinary(buffer));
    return vm;
}