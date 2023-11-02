#ifndef _DASHLE_MEMORY_H
#define _DASHLE_MEMORY_H

#include "dashle/Base.h"

#include <memory>

namespace dashle::memory {

/*
    Represents a memory range which has been allocated for target use.
    Each memory block maps to a contiguous block of memory in the address space of the host process.
    Memory blocks are unique, no two blocks ever overlap for both virtual and host range.
*/
class MemoryBlock final {
    friend class Allocator;

    uaddr m_HostBase = 0u;
    uaddr m_VirtualBase = 0u;
    usize m_Size = 0u;
    bool m_ReadOnly = false;

    MemoryBlock() {}

public:
    uaddr hostBase() const { return m_HostBase; }
    uaddr virtualBase() const { return m_VirtualBase; }
    usize size() const { return m_Size; }
    bool readOnly() const { return m_ReadOnly; }

    void setHostBase(uaddr hbase) { m_HostBase = hbase; }
    void setVirtualBase(uaddr vbase) { m_VirtualBase = vbase; }
    void setSize(usize size) { m_Size = size; }
    void setReadOnly(bool readOnly) { m_ReadOnly = readOnly; }

    Expected<uaddr> virtualToHost(uaddr vaddr) const;

    // Read from host buffer.
    bool read(uaddr hfrom, uaddr vto, usize size) const;

    // Write to host buffer.
    bool write(uaddr vfrom, uaddr hto, usize size) const;

    /* Optimized methods */

    u8 read8(uaddr vaddr) const;
    u16 read16(uaddr vaddr) const;
    u32 read32(uaddr vaddr) const;
    u64 read64(uaddr vaddr) const;
    bool write8(uaddr vaddr, u8 value) const;
    bool write16(uaddr vaddr, u16 value) const;
    bool write32(uaddr vaddr, u32 value) const;
    bool write64(uaddr vaddr, u64 value) const;
};

struct AllocatorData;

class Allocator {
private:
    std::unique_ptr<AllocatorData> m_Data;
    usize m_MaxMemory;

    // These functions are responsible for updating memory blocks and the memory counter.
    virtual bool hostAlloc(MemoryBlock& block) = 0;
    virtual void hostFree(MemoryBlock& block) = 0;

    void initialize();

    static MemoryBlock createMemoryBlock(uaddr hostBase, uaddr virtualBase, usize size, bool readOnly) {
        MemoryBlock b;
        b.m_HostBase = hostBase;
        b.m_VirtualBase = virtualBase;
        b.m_Size = size;
        b.m_ReadOnly = readOnly;
        return b;
    }

protected:
    Allocator(usize maxMemory);
    ~Allocator();

public:
    usize maxMemory() const { return m_MaxMemory; }
    virtual usize usedMemory() const = 0;
    virtual usize availableMemory() const = 0;

    // Must be explicitly called to avoid memory leaks.
    void deleteAllMemBlocks();
    
    // Delete all memory blocks and reset allocator state.
    virtual void reset();

    const MemoryBlock* blockFromVAddr(uaddr vaddr) const;

    virtual Expected<uaddr> allocate(usize size, bool readOnly = false);
    virtual Optional free(uaddr vbase);
};

/*
    Generic allocator, defaults to malloc() and free().
*/
class GenericAllocator final : public Allocator {
    usize m_UsedMemory = 0u;

    bool hostAlloc(MemoryBlock& block) override;
    void hostFree(MemoryBlock& block) override;

public:
    GenericAllocator(usize maxMemory) : Allocator(maxMemory) {}

    usize usedMemory() const override {
        return m_UsedMemory;
    }

    usize availableMemory() const override {
        return maxMemory() - usedMemory();
    }
};

} // namespace dashle::memory

#endif /* _DASHLE_MEMORY_H */