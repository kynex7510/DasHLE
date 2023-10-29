#ifndef _DASHLE_MEMORY_H
#define _DASHLE_MEMORY_H

#include "Base.h"

#include <set>

namespace dashle::memory {

/*
    Represents a memory range which has been reserved by the allocator.
    Each memory block maps to a contiguous block of memory in the address space of the host process.
    Memory blocks are unique, no two blocks ever overlap for both virtual and host range.
*/
class MemoryBlock {
    friend class Allocator;

    uaddr m_HostBase = 0u;
    uaddr m_VirtualBase = 0u;
    usize m_Size = 0u;
    bool m_ReadOnly = false;

public:
    uaddr hostBase() const { return m_HostBase; }
    uaddr virtualBase() const { return m_VirtualBase; }
    usize size() const { return m_Size; }
    bool readOnly() const { return m_ReadOnly; }

    uaddr virtualToHost(uaddr vaddr);

    // Read from host buffer.
    bool read(uaddr hfrom, uaddr vto, usize size);

    // Write to host buffer.
    bool write(uaddr vfrom, uaddr hto, usize size);

    /* Optimized methods */

    u8 read8(uaddr vaddr);
    u16 read16(uaddr vaddr);
    u32 read32(uaddr vaddr);
    u64 read64(uaddr vaddr);
    void write8(uaddr vaddr, u8 value);
    void write16(uaddr vaddr, u16 value);
    void write32(uaddr vaddr, u32 value);
    void write64(uaddr vaddr, u64 value);
};

/*
    Comparator for memory blocks, acts like std::less in the general case.
    When size = 0 we're comparing the block with a virtual address.
*/
struct MemoryBlockComparator {
    constexpr bool operator()(const MemoryBlock &a, const MemoryBlock &b) {
        if (!a.size()) {
            // A is the virtual address. Comp(A, B) returns true if A < B.
            // Hence comp(A, B) = vaddr < block.base.
            return a.virtualBase() < b.virtualBase();
        }

        if (!b.size()) {
            // B is the virtual address. Comp(A, B) returns true if A < B.
            // Hence comp(A, B) = (block.base + block.size) < vaddr.
            return (a.virtualBase() + a.size()) < b.virtualBase();
        }

        // Generic case: both A and B are blocks.
        // We know they can't overlap, hence comp(A, B) = A.base < B.base.
        return a.virtualBase() < b.virtualBase();
    }
};

class Allocator {
public:
    using MemoryBlockSet = std::set<MemoryBlock, MemoryBlockComparator>;

private:
    // MemoryBlockSet m_MemBlocks;

    /*
    auto find(uaddr vaddr) const {
        MemoryBlock b;
        b.m_VirtualBase = vaddr;
        return m_MemBlocks.find(b);
    }
    */

    virtual uaddr hostAlloc(usize size) = 0;
    virtual void hostFree(uaddr addr) = 0;

public:
    virtual usize usedMemory() const = 0;
    virtual usize availableMemory() const = 0;
    virtual usize maxMemory() const = 0;

    void reset();

    /*
    MemoryBlock *blockFromVAddr(uaddr vaddr) const {
        auto it = find(vaddr);
        return it != m_MemBlocks.end() ? const_cast<MemoryBlock *>(&*it) : nullptr;
    }
    */

    // TODO
    uaddr allocate(uaddr hint, usize size, bool readOnly) {
        return hostAlloc(size);
    }

    bool free(uaddr vbase) {
        hostFree(vbase);
        return true;
    }

    uaddr resize(uaddr vbase, usize newSize) { return 0u; }
};

} // namespace dashle::memory

#endif /* _DASHLE_MEMORY_H */