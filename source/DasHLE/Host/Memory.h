#ifndef _DASHLE_HOST_MEMORY_H
#define _DASHLE_HOST_MEMORY_H

#include "DasHLE/Base.h"

#include <memory>

namespace dashle::host::memory {

namespace flags {

constexpr static usize READ  = 0b001;
constexpr static usize WRITE = 0b010;
constexpr static usize EXEC  = 0b100;

constexpr static usize READ_WRITE = READ | WRITE;
constexpr static usize MASK  = EXEC | READ_WRITE;

}; // namespace dashle::host::memory::flags

struct AllocatedBlock {
    uaddr hostBase = 0u;
    uaddr virtualBase = 0u;
    usize size = 0u;
    usize flags = 0u;
};

class HostAllocator {
public:
    virtual ~HostAllocator() {}
    virtual bool initialize() { return true; }
    virtual void finalize() {}
    virtual bool alloc(AllocatedBlock& block);
    virtual void free(AllocatedBlock& block);
};

class MemoryManager {
    struct Data;
    
    std::unique_ptr<HostAllocator> m_HostAllocator;
    std::unique_ptr<Data> m_Data;
    const usize m_MaxMemory = 0u;
    usize m_UsedMemory = 0u;
    uaddr m_Offset = 0u;

    void initialize();
    void finalize();
    bool hostAlloc(AllocatedBlock& block);
    void hostFree(AllocatedBlock& block);

public:
    MemoryManager(std::unique_ptr<HostAllocator> allocator, usize maxMemory, usize offset = 0u);
    ~MemoryManager();

    usize maxMemory() const { return m_MaxMemory; }
    usize usedMemory() const { return m_UsedMemory; }
    usize availableMemory() const { return maxMemory() - usedMemory(); }
    usize virtualOffset() const { return m_Offset; }

    // Return an always invalid virtual address.
    uaddr invalidAddr() const { return virtualOffset() + maxMemory(); }
    
    // Free all allocated memory and reset internal state.
    void reset();

    // Get allocated block from virtual address.
    Expected<const AllocatedBlock*> blockFromVAddr(uaddr vaddr) const;

    // Allocate memory, return the virtual address.
    Expected<uaddr> allocate(uaddr hint, usize size, usize flags);

    Expected<uaddr> allocate(usize size, usize flags = flags::READ | flags::WRITE) {
        return allocate(invalidAddr(), size, flags);
    }

    // Free allocated memory.
    Expected<void> free(uaddr vbase);

    // Set memory flags.
    Expected<void> setFlags(uaddr vbase, usize flags);
};

// Translate a virtual address to an host address.
inline Expected<uaddr> virtualToHost(const AllocatedBlock& block, uaddr vaddr) {
    if (vaddr >= block.virtualBase && vaddr <= (block.virtualBase + block.size))
        return vaddr - block.virtualBase + block.hostBase;

    return Unexpected(Error::InvalidAddress);
}

} // namespace dashle::host::memory

#endif /* _DASHLE_HOST_MEMORY_H */