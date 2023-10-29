#include "DasHLE.h"

#include <algorithm>

using namespace dashle;
using namespace dashle::memory;

// MemoryBlock

uaddr MemoryBlock::virtualToHost(uaddr vaddr) {
    const auto vbase = virtualBase();
    if (vaddr >= vbase && vaddr <= (vbase + size()))
        return vaddr - vbase + hostBase();

    return 0u;
}

bool MemoryBlock::read(uaddr hfrom, uaddr vto, usize size) {
    if (auto haddr = virtualToHost(vto)) {
        std::copy(hfrom, hfrom + size, haddr);
        return true;
    }

    return false;
}

bool MemoryBlock::write(uaddr vfrom, uaddr hto, usize size) {
    if (auto haddr = virtualToHost(vfrom)) {
        std::copy(haddr, haddr + size, hto);
        return true;
    }

    return false;
}

u8 MemoryBlock::read8(uaddr addr) {
    auto haddr = virtualToHost(addr);
    DASHLE_ASSERT(haddr);
    return *reinterpret_cast<u8 *>(haddr);
}

u16 MemoryBlock::read16(uaddr addr) {
    auto haddr = virtualToHost(addr);
    DASHLE_ASSERT(haddr);
    return *reinterpret_cast<u16 *>(haddr);
}

u32 MemoryBlock::read32(uaddr addr) {
    auto haddr = virtualToHost(addr);
    DASHLE_ASSERT(haddr);
    return *reinterpret_cast<u32 *>(haddr);
}

u64 MemoryBlock::read64(uaddr addr) {
    auto haddr = virtualToHost(addr);
    DASHLE_ASSERT(haddr);
    return *reinterpret_cast<u64 *>(haddr);
}

void MemoryBlock::write8(uaddr addr, u8 value) {
    if (!readOnly()) {
        auto haddr = virtualToHost(addr);
        DASHLE_ASSERT(haddr);
        *reinterpret_cast<u8 *>(haddr) = value;
    }
}

void MemoryBlock::write16(uaddr addr, u16 value) {
    if (!readOnly()) {
        auto haddr = virtualToHost(addr);
        DASHLE_ASSERT(haddr);
        *reinterpret_cast<u16 *>(haddr) = value;
    }
}

void MemoryBlock::write32(uaddr addr, u32 value) {
    if (!readOnly()) {
        auto haddr = virtualToHost(addr);
        DASHLE_ASSERT(haddr);
        *reinterpret_cast<u32 *>(haddr) = value;
    }
}

void MemoryBlock::write64(uaddr addr, u64 value) {
    if (!readOnly()) {
        auto haddr = virtualToHost(addr);
        DASHLE_ASSERT(haddr);
        *reinterpret_cast<u64 *>(haddr) = value;
    }
}