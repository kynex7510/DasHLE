#include "DasHLE/Guest/Context.h"

using namespace dashle;

constexpr const char BINARY_PATH[] = "/home/user/Documents/repos/DasHLE/app/lib/armeabi-v7a/libcocos2dcpp.so";

int main() {
    auto ret = dashle::guest::createContext(BINARY_PATH);
    if (!ret) {
        DASHLE_LOG_LINE("Could not open binary ({})", ret.error());
        return 1;
    }

    auto ctx = std::move(ret.value());
    ctx->dump();
    return 0;
}