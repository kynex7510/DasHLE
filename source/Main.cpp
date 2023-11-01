#include "dynarmic/interface/A32/a32.h"

#include "DasHLE.h"

#include <iostream>
#include <format>

using namespace dashle;

class MyBinary final : public binary::Binary {
    bool fixDataRelocation() override { return true; }
    bool fixCodeRelocation() override { return true; }

public:
    MyBinary(memory::Allocator &allocator) : Binary(allocator) {}
};

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

class MyEnvironment final : public Dynarmic::A32::UserCallbacks {
    MyBinary m_Binary;

public:
    MyEnvironment(memory::Allocator &allocator) : m_Binary(allocator) {}

    bool setup(stdfs::path const &path) {
        auto error = m_Binary.load(path);

        if (error == binary::Error::Success) {
            std::cout << "Binary loaded!\n";
            std::cout << std::format("- Type: {}\n", binary::typeAsString(m_Binary.type()));
            std::cout << std::format("- Base address: 0x{:X}\n", m_Binary.baseAddress());
            std::cout << std::format("- Size: 0x{:X}\n", m_Binary.size());
        } else {
            std::cout << std::format("Error: {}\n", binary::errorAsString(error));
            return 1;
        }

        return true;
    }

    std::uint8_t MemoryRead8(std::uint32_t vaddr) override {
        if (vaddr >= m_Binary.size()) {
            return 0;
        }
        return m_Binary.codeBuffer()[vaddr];
    }

    std::uint16_t MemoryRead16(std::uint32_t vaddr) override {
        return MemoryRead8(vaddr) | static_cast<std::uint16_t>(MemoryRead8(vaddr + 1)) << 8;
    }

    std::uint32_t MemoryRead32(std::uint32_t vaddr) override {
        return std::uint32_t(MemoryRead16(vaddr)) | std::uint32_t(MemoryRead16(vaddr + 2)) << 16;
    }

    std::uint64_t MemoryRead64(std::uint32_t vaddr) override {
        return std::uint64_t(MemoryRead32(vaddr)) | std::uint64_t(MemoryRead32(vaddr + 4)) << 32;
    }

    void MemoryWrite8(u32 vaddr, u8 value) override {
        if (vaddr >= m_Binary.size()) {
            return;
        }
        m_Binary.codeBuffer()[vaddr] = value;
    }

    void MemoryWrite16(u32 vaddr, u16 value) override {
        MemoryWrite8(vaddr, u8(value));
        MemoryWrite8(vaddr + 1, u8(value >> 8));
    }

    void MemoryWrite32(u32 vaddr, u32 value) override {
        MemoryWrite16(vaddr, u16(value));
        MemoryWrite16(vaddr + 2, u16(value >> 16));
    }

    void MemoryWrite64(u32 vaddr, u64 value) override {
        MemoryWrite32(vaddr, u32(value));
        MemoryWrite32(vaddr + 4, u32(value >> 32));
    }

    void InterpreterFallback(u32 pc, size_t num_instructions) override {
        // This is never called in practice.
        std::terminate();
    }

    void CallSVC(u32 swi) override {
        // Do something.
    }

    void ExceptionRaised(u32 pc, Dynarmic::A32::Exception exception) override {
        std::cout << "Exception raised: " << (std::uint32_t)exception << "\n";
    }

    void AddTicks(u64 ticks) override {}

    u64 GetTicksRemaining() override { return -1; }
};

int main() {
    memory::GenericAllocator32 allocator;
    allocator.reset();

    usize numAllocs = 0;
    srand(time(nullptr));
    std::cout << std::hex;

    while (true) {
        auto const size = (rand() % 0x1000000) + 1;
        std::cout << ++numAllocs << ") Allocating " << size << " bytes... ";
        auto p = allocator.allocate(size, false);
        if (!p) {
            std::cout << "failed (" << (u32)p.error() << ")\n";
            std::cout << "Total allocated bytes: " << allocator.usedMemory() << '\n';
            std::cout << std::format("{}% memory used\n", ((double)allocator.usedMemory() / allocator.maxMemory()) * 100);
            break;
        }

        std::cout << "success: " << p.value() << "\n";

        if (rand() & 1) {
            std::cout << "- Freeing... ";
            auto o = allocator.free(p.value());
            if (o) {
                std::cout << "failed (" << (u32)o.value() << ")\n";
                break;
            }

            std::cout << "success\n";
        }
        // allocator.debug();
    }

    /*
    allocator.reset();
    MyEnvironment env(allocator);
    if (!env.setup("/home/user/Documents/repos/DasHLE/app/lib/armeabi-v7a/libcocos2dcpp.so"))
        return 1;

    std::uint8_t stack[0x1000];
    const auto offset = 0x2985AA;

    Dynarmic::A32::UserConfig user_config;
    user_config.callbacks = &env;
    Dynarmic::A32::Jit cpu(user_config);

    cpu.Regs()[15] = offset;
    cpu.SetCpsr(0x00000030); // Thumb mode
    // cpu.DumpDisassembly();
    auto reason = cpu.Run();
    std::cout << "Reason: " << (u32)reason << '\n';
    printf("R0: %u\n", cpu.Regs()[0]);
    */

    return 0;
}