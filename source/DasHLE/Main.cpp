#include "DasHLE/Emu/ARM/EmuContext.h"

using namespace dashle;
namespace arm = dashle::emu::arm;

int main() {
    arm::EmuContext ctx;

    auto ret = ctx.openBinary("/home/user/Documents/repos/DasHLE/app/lib/armeabi-v7a/libcocos2dcpp.so");
    if (!ret) {
        DASHLE_LOG_LINE("Could not open binary ({})", errorAsString(ret.error()));
        return 1;
    }

    const auto env = ctx.env();
    const auto binaryBase = env->binaryBase().value();
    DASHLE_LOG_LINE("Binary base: 0x{:X}", binaryBase);
    DASHLE_LOG_LINE("Stack base: 0x{:X}", env->stackBase().value());

    ctx.initCpu();
    ctx.execute(binaryBase + 0x2985AB);
    DASHLE_LOG_LINE("R0: 0x{:X}", ctx.getRegister(arm::REG_R0));
    return 0;
}