#include "GDACL.h"

#include <iostream>
#include <format>

using namespace gdacl;

class MyBinary final : public binary::Base {
    std::uintptr_t resolveDependency(const std::string &sym) override {
        return 0;
    }
};

int main() {
    auto myBinary = std::make_unique<MyBinary>();
    auto error = myBinary->load("/home/user/Documents/repos/gdacl/app/lib/armeabi-v7a/libcocos2dcpp.so");

    if (error == binary::Error::Success) {
        std::cout << "Binary loaded!\n";
        std::cout << std::format("- Type: {}\n", binary::getTypeAsString(myBinary->getType()));
        std::cout << std::format("- Base address: 0x{:X}\n", myBinary->getBaseAddress());
        std::cout << std::format("- Size: 0x{:X}\n", myBinary->getSize());
    } else {
        std::cout << std::format("Error: {}\n", binary::getErrorAsString(error));
    }

    return 0;
}