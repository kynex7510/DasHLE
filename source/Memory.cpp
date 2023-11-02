#include "DasHLE.h"

#include <set>
#include <algorithm>
#include <iterator>
#include <cstdlib>

using namespace dashle;
using namespace dashle::memory;

static void copyBytes(uaddr from, uaddr to, usize size) {
    std::copy(reinterpret_cast<u8*>(from), 
        reinterpret_cast<u8*>(from) + size,
        reinterpret_cast<u8*>(to));
}

// MemoryBlock

Expected<uaddr> MemoryBlock::virtualToHost(uaddr vaddr) const {
    const auto vbase = virtualBase();
    if (vaddr >= vbase && vaddr <= (vbase + size()))
        return vaddr - vbase + hostBase();

    return std::unexpected(Error::InvalidAddress);
}

bool MemoryBlock::read(uaddr hfrom, uaddr vto, usize size) const {
    if (auto haddr = virtualToHost(vto)) {
        copyBytes(hfrom, hfrom + size, haddr.value());
        return true;
    }

    return false;
}

bool MemoryBlock::write(uaddr vfrom, uaddr hto, usize size) const {
    if (auto haddr = virtualToHost(vfrom); haddr && !readOnly()) {
        copyBytes(haddr.value(), haddr.value() + size, hto);
        return true;
    }

    return false;
}

u8 MemoryBlock::read8(uaddr addr) const {
    auto haddr = virtualToHost(addr);
    DASHLE_ASSERT(haddr);
    return *reinterpret_cast<u8*>(haddr.value());
}

u16 MemoryBlock::read16(uaddr addr) const {
    auto haddr = virtualToHost(addr);
    DASHLE_ASSERT(haddr);
    return *reinterpret_cast<u16*>(haddr.value());
}

u32 MemoryBlock::read32(uaddr addr) const {
    auto haddr = virtualToHost(addr);
    DASHLE_ASSERT(haddr);
    return *reinterpret_cast<u32*>(haddr.value());
}

u64 MemoryBlock::read64(uaddr addr) const {
    auto haddr = virtualToHost(addr);
    DASHLE_ASSERT(haddr);
    return *reinterpret_cast<u64*>(haddr.value());
}

bool MemoryBlock::write8(uaddr addr, u8 value) const {
    if (!readOnly()) {
        auto haddr = virtualToHost(addr);
        DASHLE_ASSERT(haddr);
        *reinterpret_cast<u8*>(haddr.value()) = value;
        return true;
    }

    return false;
}

bool MemoryBlock::write16(uaddr addr, u16 value) const {
    if (!readOnly()) {
        auto haddr = virtualToHost(addr);
        DASHLE_ASSERT(haddr);
        *reinterpret_cast<u16*>(haddr.value()) = value;
        return true;
    }

    return false;
}

bool MemoryBlock::write32(uaddr addr, u32 value) const {
    if (!readOnly()) {
        auto haddr = virtualToHost(addr);
        DASHLE_ASSERT(haddr);
        *reinterpret_cast<u32*>(haddr.value()) = value;
        return true;
    }

    return false;
}

bool MemoryBlock::write64(uaddr addr, u64 value) const {
    if (!readOnly()) {
        auto haddr = virtualToHost(addr);
        DASHLE_ASSERT(haddr);
        *reinterpret_cast<u64*>(haddr.value()) = value;
        return true;
    }

    return false;
}

// Allocator

struct FreeBlock {
    uaddr virtualBase = 0u;
    usize size = 0u;
};

struct MemoryBlockComparator {
    constexpr bool operator()(const MemoryBlock& a, const MemoryBlock& b) const {
        // When size = 0 we're comparing the block with a virtual address.

        if (!a.size()) {
            // A is the virtual address. Comp(A, B) returns true if A < B.
            // Hence comp(A, B) = vaddr < block.base.
            return a.virtualBase() < b.virtualBase();
        }

        if (!b.size()) {
            // B is the virtual address. Comp(A, B) returns true if A < B.
            // Hence comp(A, B) = (block.base + block.size) <= vaddr.
            return (a.virtualBase() + a.size()) <= b.virtualBase();
        }

        // Generic case: both A and B are blocks.
        // We know they can't overlap, hence comp(A, B) = A.base < B.base.
        return a.virtualBase() < b.virtualBase();
    }
};

struct FreeBlockComparator {
    constexpr bool operator()(const FreeBlock& a, const FreeBlock& b) const {
        // If the size is the same, sort by smallest virtual base.
        if (a.size == b.size)
            return a.virtualBase < b.virtualBase;

        return a.size < b.size;
    };
};

using MemoryBlockSet = std::set<MemoryBlock, MemoryBlockComparator>;
using FreeBlockSet = std::set<FreeBlock, FreeBlockComparator>;

struct dashle::memory::AllocatorData {
    MemoryBlockSet memBlocks;
    FreeBlockSet freeBlocks;
};

void Allocator::initialize() {
    // Add main free block.
    const auto pair = m_Data->freeBlocks.insert(FreeBlock{.size = maxMemory()});
    DASHLE_ASSERT(pair.second); // Shouldn't happen.
}

Allocator::Allocator(usize maxMemory) : m_MaxMemory(maxMemory) {
    m_Data = std::make_unique<dashle::memory::AllocatorData>();
    initialize();
}

Allocator::~Allocator() {} // Required by the destructor of data.

void Allocator::deleteAllMemBlocks() {
    auto it = m_Data->memBlocks.begin();
    while (it != m_Data->memBlocks.end()) {
        auto node = m_Data->memBlocks.extract(it);
        hostFree(node.value());
        it = m_Data->memBlocks.begin();
    }
}

void Allocator::reset() {
    deleteAllMemBlocks();
    m_Data->freeBlocks.clear();
    initialize();
}

const MemoryBlock* Allocator::blockFromVAddr(uaddr vaddr) const {
    auto it = m_Data->memBlocks.find(createMemoryBlock(0u, vaddr, 0u, false));
    return it != m_Data->memBlocks.end() ? &*it : nullptr;
}

Expected<uaddr> Allocator::allocate(usize size, bool readOnly) {
    if (!size)
        return Unexpected(Error::InvalidSize);

    if (availableMemory() < size)
        return Unexpected(Error::NoVirtualMemory);

    // Find the smallest block that can hold size bytes (best-fit).
    // This is optimized for small allocations.
    auto it = m_Data->freeBlocks.lower_bound(FreeBlock({.size = size}));
    if (it == m_Data->freeBlocks.end())
        return Unexpected(Error::NoVirtualMemory);

    // Create memory block.
    auto freeBlockNode = m_Data->freeBlocks.extract(it);
    auto& freeBlock = freeBlockNode.value();
    auto memBlock = createMemoryBlock(0u, freeBlock.virtualBase, size, readOnly);
    if (!hostAlloc(memBlock)) {
        m_Data->freeBlocks.insert(std::move(freeBlockNode));
        return std::unexpected(Error::NoHostMemory);
    }

    // Update the free block.
    freeBlock.virtualBase += size;
    freeBlock.size -= size;
    if (freeBlock.size)
        m_Data->freeBlocks.insert(std::move(freeBlockNode));

    // Add the new memory block.
    m_Data->memBlocks.insert(memBlock);
    return memBlock.virtualBase();
}

Optional Allocator::free(uaddr vbase) {
    // Find memory block.
    auto it = m_Data->memBlocks.find(createMemoryBlock(0u, vbase, 0u, false));
    if (it == m_Data->memBlocks.end() || it->virtualBase() != vbase)
        return Error::NoMemoryBlock;

    // Handle merging with previous and next blocks.
    auto newFreeBlock = FreeBlock{
        .virtualBase = it->virtualBase(),
        .size = it->size()};

    const auto handleBlock = [&](AllocatorData* data, const FreeBlock& block, bool next) {
        const auto freeBlockIt = data->freeBlocks.find(block);
        DASHLE_ASSERT(freeBlockIt != data->freeBlocks.end()); // Shouldn't happen.
        // Update new free block.
        if (!next)
            newFreeBlock.virtualBase = freeBlockIt->virtualBase;
        newFreeBlock.size += freeBlockIt->size;
        // Erase old free block.
        data->freeBlocks.erase(freeBlockIt);
    };

    if (it != m_Data->memBlocks.begin()) {
        // Pick the previous block from the set, which is necessarily an allocated block. Since we sort by
        // virtual address, we know it's also the previous block in the address space.
        // By the way the allocation algorithm works, between two memory blocks there can only be a single
        // free block, so either the previous block end matches with our block start, or it matches with the
        // start of a free block. In the latter case we will merge the two blocks.
        const auto prevAllocBlock = std::prev(it);
        const auto prevAllocBlockEnd = prevAllocBlock->virtualBase() + prevAllocBlock->size();
        if (prevAllocBlockEnd != it->virtualBase()) {
            const auto prevFreeBlock = FreeBlock{
                .virtualBase = prevAllocBlockEnd,
                .size = it->virtualBase() - prevAllocBlockEnd};
            handleBlock(m_Data.get(), prevFreeBlock, false);
        }
    } else {
        // Handle the case where this is the first memory block of the set but not the first block in
        // the address space (ie. address space starts with a free block).
        if (it->virtualBase() != 0u) {
            const auto firstFreeBlock = FreeBlock{
                .virtualBase = 0u,
                .size = it->virtualBase()};
            handleBlock(m_Data.get(), firstFreeBlock, false);
        }
    }

    if (const auto nextAllocBlock = std::next(it); nextAllocBlock != m_Data->memBlocks.end()) {
        // Same as above, except we check for the next memory block.
        const auto thisAllocBlockEnd = it->virtualBase() + it->size();
        if (thisAllocBlockEnd != nextAllocBlock->virtualBase()) {
            const auto nextFreeBlock = FreeBlock{
                .virtualBase = thisAllocBlockEnd,
                .size = nextAllocBlock->virtualBase() - thisAllocBlockEnd};
            handleBlock(m_Data.get(), nextFreeBlock, true);
        }
    } else {
        // Handle the case where this is the last allocated block, but not the last in memory (ie. last
        // block is a free one).
        if ((it->virtualBase() + it->size()) != maxMemory()) {
            const auto thisAllocBlockEnd = it->virtualBase() + it->size();
            const auto lastFreeBlock = FreeBlock{
                .virtualBase = thisAllocBlockEnd,
                .size = maxMemory() - thisAllocBlockEnd};
            handleBlock(m_Data.get(), lastFreeBlock, true);
        }
    }

    // Free host space and add new free block.
    auto node = m_Data->memBlocks.extract(it);
    hostFree(node.value());
    m_Data->freeBlocks.insert(newFreeBlock);
    return OPTIONAL_SUCCESS;
}

// GenericAllocator

bool GenericAllocator::hostAlloc(MemoryBlock& block) {
    if (auto addr = std::malloc(block.size())) {
        block.setHostBase(reinterpret_cast<uaddr>(addr));
        m_UsedMemory += block.size();
        return true;
    }

    return false;
}

void GenericAllocator::hostFree(MemoryBlock& block) {
    std::free(reinterpret_cast<void*>(block.hostBase()));
    block.setHostBase(0u);
    m_UsedMemory -= block.size();
}