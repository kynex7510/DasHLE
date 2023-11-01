#ifndef _DASHLE_MEMORY_H
#define _DASHLE_MEMORY_H

#include "Base.h"

#include <memory>
#include <expected>
#include <optional>

namespace dashle::memory {

enum class Error {
    InvalidSize,
    NoVirtualMemory,
    NoHostMemory,
    NoMemoryBlock,
};

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

    uaddr virtualToHost(uaddr vaddr) const;

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

    // These functions are responsible for updating the block and the memory counter.
    virtual bool hostAlloc(MemoryBlock &block) = 0;
    virtual void hostFree(MemoryBlock &block) = 0;

    void freeAllMemBlocks();

    static MemoryBlock createMemoryBlock(uaddr hostBase, uaddr virtualBase, usize size, bool readOnly) {
        MemoryBlock b;
        b.m_HostBase = hostBase;
        b.m_VirtualBase = virtualBase;
        b.m_Size = size;
        b.m_ReadOnly = readOnly;
        return b;
    }

public:
    Allocator();
    ~Allocator();

    virtual usize usedMemory() const = 0;
    virtual usize availableMemory() const = 0;
    virtual usize maxMemory() const = 0;

    virtual void reset();
    virtual const MemoryBlock *blockFromVAddr(uaddr vaddr) const;

    virtual std::expected<uaddr, Error> allocate(usize size, bool readOnly = false);
    virtual std::optional<Error> free(uaddr vbase);
};

class GenericAllocator : public Allocator {
    usize m_AddrSize = 0u;
    usize m_UsedMemory = 0u;

    bool hostAlloc(MemoryBlock &block) override;
    void hostFree(MemoryBlock &block) override;

public:
    GenericAllocator(usize addressSize) : m_AddrSize(addressSize) {}

    usize usedMemory() const override {
        return m_UsedMemory;
    }

    usize availableMemory() const override {
        return maxMemory() - usedMemory();
    }

    usize maxMemory() const override {
        return static_cast<usize>(1) << m_AddrSize;
    }
};

class GenericAllocator32 final : public GenericAllocator {
public:
    GenericAllocator32() : GenericAllocator(32u) {}
};

class GenericAllocator64 final : public GenericAllocator {
public:
    GenericAllocator64() : GenericAllocator(64u) {}
};

} // namespace dashle::memory

#endif /* _DASHLE_MEMORY_H */