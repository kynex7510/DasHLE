#include "DasHLE/Host/Memory.h"

#include <set>
#include <algorithm>
#include <iterator>
#include <cstdlib>

using namespace dashle;
using namespace dashle::host::memory;

// HostAllocator

bool HostAllocator::alloc(AllocatedBlock& block) {
    if (auto addr = std::malloc(block.size)) {
        block.hostBase = reinterpret_cast<uaddr>(addr);
        return true;
    }

    return false;
}

void HostAllocator::free(AllocatedBlock& block) {
    std::free(reinterpret_cast<void*>(block.hostBase));
    block.hostBase = 0u;
}

// MemoryManager

struct FreeBlock {
    uaddr virtualBase = 0u;
    usize size = 0u;
};

struct AllocatedBlockComparator {
    bool operator()(const AllocatedBlock& a, const AllocatedBlock& b) const {
        // When size = 0 we're comparing the block with a virtual address.

        if (!a.size) {
            // A is the virtual address. Comp(A, B) returns true if A < B.
            // Hence comp(A, B) = vaddr < block.base.
            return a.virtualBase < b.virtualBase;
        }

        if (!b.size) {
            // B is the virtual address. Comp(A, B) returns true if A < B.
            // Hence comp(A, B) = (block.base + block.size) <= vaddr.
            return (a.virtualBase + a.size) <= b.virtualBase;
        }

        // Generic case: both A and B are blocks.
        // We know they can't overlap, hence comp(A, B) = A.base < B.base.
        return a.virtualBase < b.virtualBase;
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

using AllocatedBlockSet = std::set<AllocatedBlock, AllocatedBlockComparator>;
using FreeBlockSet = std::set<FreeBlock, FreeBlockComparator>;

struct dashle::host::memory::MemoryManager::Data {
    AllocatedBlockSet allocatedBlocks;
    FreeBlockSet freeBlocks;
};

void MemoryManager::initialize() {
    // Initialize allocator.
    DASHLE_ASSERT(m_HostAllocator);
    DASHLE_ASSERT(m_HostAllocator->initialize());

    // Add main free block.
    const auto pair = m_Data->freeBlocks.insert({
        .virtualBase = m_Offset,
        .size = maxMemory()});
    DASHLE_ASSERT(pair.second);
}

void MemoryManager::finalize() {
    DASHLE_ASSERT(m_Data);

    // Free all allocated memory.
    auto& allocatedBlocks = m_Data->allocatedBlocks;
    auto it = allocatedBlocks.begin();
    while (it != allocatedBlocks.end()) {
        auto node = allocatedBlocks.extract(it);
        m_HostAllocator->free(node.value());
        it = allocatedBlocks.begin();
    }

    // Remove memory blocks.
    m_Data->freeBlocks.clear();

    // Finalize allocator.
    m_HostAllocator->finalize();
}

bool MemoryManager::hostAlloc(AllocatedBlock& block) {
    DASHLE_ASSERT(m_HostAllocator);

    const auto originalVBase = block.virtualBase;
    const auto originalSize = block.size;
    const auto originalFlags = block.flags;

    bool ret = m_HostAllocator->alloc(block);

    // Allocators shall not modify block values.
    DASHLE_ASSERT(block.virtualBase == originalVBase);
    DASHLE_ASSERT(block.size == originalSize);
    DASHLE_ASSERT(block.flags == originalFlags);

    return ret;
}

void MemoryManager::hostFree(AllocatedBlock& block) {
    DASHLE_ASSERT(m_HostAllocator);
    m_HostAllocator->free(block);
}

MemoryManager::MemoryManager(std::unique_ptr<HostAllocator> allocator, usize size, usize width, usize offset)
    : m_HostAllocator(std::move(allocator)), m_MaxMemory(size), m_Width(width), m_Offset(offset) {
    DASHLE_ASSERT(addressWidth() < 64);

    const auto maxAddr = static_cast<usize>(1u) << addressWidth();
    DASHLE_ASSERT(virtualOffset() + maxMemory() <= maxAddr);

    m_Data = std::make_unique<dashle::host::memory::MemoryManager::Data>();
    initialize();
}

MemoryManager::~MemoryManager() { finalize(); }

void MemoryManager::reset() {
    finalize();
    initialize();
}

Expected<const AllocatedBlock*> MemoryManager::blockFromVAddr(uaddr vaddr) const {
    const auto& allocatedBlocks = m_Data->allocatedBlocks;
    const auto it = allocatedBlocks.find({ .virtualBase = vaddr });

    if (it != allocatedBlocks.end())
        return &*it;

    return Unexpected(Error::NotFound);
}

static Expected<uaddr> alignedBase(uaddr base, usize alignment) {
    DASHLE_TRY_EXPECTED_CONST(alignedDown, dashle::align(base, alignment));

    // If the address is aligned we don't have to do anything.
    if (alignedDown != base) {
        DASHLE_ASSERT_WRAPPER_CONST(alignedUp, dashle::alignOver(base, alignment));
        return alignedUp;
    }

    return alignedDown;
}

static Expected<FreeBlockSet::iterator> findFreeBlock(FreeBlockSet& freeBlocks, usize requestedSize, usize alignment) {
    // Find the smallest block that can hold size bytes (best-fit).
    // This is optimized for small allocations.
    auto it = freeBlocks.lower_bound({.size = requestedSize});
    if (it == freeBlocks.end())
        return Unexpected(Error::NoVirtualMemory);

    // Check that we have enough space for alignment.
    if (alignment) {   
        do {
            DASHLE_ASSERT_WRAPPER_CONST(alignedAddr, alignedBase(it->virtualBase, alignment));
            const auto avalSize = it->size - (alignedAddr - it->virtualBase);
            if (avalSize >= requestedSize)
                break;
        } while (++it != freeBlocks.end());

        if (it == freeBlocks.end())
            return Unexpected(Error::NoVirtualMemory);
    }

    return it;
}

Expected<uaddr> MemoryManager::findFreeAddr(usize size, usize alignment) const {
    return findFreeBlock(m_Data->freeBlocks, size, alignment).and_then([alignment](FreeBlockSet::iterator it) -> Expected<uaddr> {
        if (alignment) {
            // If we got there, the alignment has to be valid.
            DASHLE_ASSERT_WRAPPER_CONST(alignedAddr, alignedBase(it->virtualBase, alignment));
            return alignedAddr;
        }

        return it->virtualBase;
    });
}

Expected<const AllocatedBlock*> MemoryManager::allocate(const AllocArgs& args) {
    auto alignment = args.alignment;

    if (!args.size)
        return Unexpected(Error::InvalidSize);

    if (availableMemory() < args.size)
        return Unexpected(Error::NoVirtualMemory);

    if (alignment && !dashle::isPowerOfTwo(alignment))
        return Unexpected(Error::InvalidAlignment);

    // If we have alignment then size must be aligned too.
    if (alignment) {
        DASHLE_ASSERT_WRAPPER_CONST(alignedSize, dashle::align(args.size, alignment));
        if (alignedSize != args.size)
            return Unexpected(Error::InvalidSize);
    }

    // If we have an enforced hint and alignment, make sure the hint is aligned.
    if (args.hint && alignment && (args.flags & flags::FORCE_HINT)) {
        DASHLE_ASSERT_WRAPPER_CONST(hint, args.hint);
        DASHLE_ASSERT_WRAPPER_CONST(alignedAddr, dashle::align(hint, alignment));
        if (hint != alignedAddr)
            return Unexpected(Error::InvalidAddress);

        // At this point alignment is already handled.
        alignment = 0;
    }

    auto& allocatedBlocks = m_Data->allocatedBlocks;
    auto& freeBlocks = m_Data->freeBlocks;
    auto allocIt = freeBlocks.end();
    uaddr allocBase = 0u;

    // Handle the case when we're given a hint.
    if (args.hint) {
        DASHLE_ASSERT_WRAPPER(hint, args.hint);

        // Find a block that contains our hint (slow!).
        auto it = freeBlocks.begin();
        while (it != freeBlocks.end()) {
            if (hint >= it->virtualBase && hint <= (it->virtualBase + it->size))
                break;

            ++it;
        }
    
        // Handle alignment and make sure that we have enough space.
        if (it != freeBlocks.end()) {
            // Alignment checks also check for available space.
            if (alignment) {
                // Attempt to align down.
                DASHLE_ASSERT_WRAPPER_CONST(alignedDownAddr, dashle::align(hint, alignment));
                if (alignedDownAddr < it->virtualBase) {
                    // Attempt to align up.
                    DASHLE_ASSERT_WRAPPER_CONST(alignedUpAddr, dashle::alignOver(hint, alignment));
                    if (alignedUpAddr > (it->virtualBase + it->size)) {
                        it = freeBlocks.end();
                    } else {
                        hint = alignedUpAddr;
                    }
                } else {
                    hint = alignedDownAddr;
                }
            } else {
                // Fallback to the base address if we don't have space and the hint isn't enforced.
                if ((it->virtualBase + it->size) - hint < args.size) {
                    if (!(args.flags & flags::FORCE_HINT)) {
                        hint = it->virtualBase;
                    } else {
                        it = freeBlocks.end();
                    }
                }
            }
        }

        // Fail if we havent found an address and the hint was enforced.
        if (it == freeBlocks.end() && (args.flags & flags::FORCE_HINT))
            return Unexpected(Error::NoVirtualMemory);
            
        allocIt = it;
        allocBase = hint;
    }
    
    // Handle the general case.
    if (allocIt == freeBlocks.end()) {
        // This function takes care of the alignment.
        DASHLE_TRY_EXPECTED(it, findFreeBlock(freeBlocks, args.size, alignment));
        allocIt = it;

        if (alignment) {
            DASHLE_ASSERT_WRAPPER_CONST(alignedAddr, alignedBase(it->virtualBase, alignment));
            allocBase = alignedAddr;
        } else {
            allocBase = it->virtualBase;
        }
    }

    // At this point allocIt is always valid.
    // Allocate block in memory.
    auto freeBlockNode = freeBlocks.extract(allocIt);
    auto& freeBlock = freeBlockNode.value();
    auto allocatedBlock = AllocatedBlock {
        .virtualBase = allocBase,
        .size = args.size,
        .flags = args.flags & flags::PERM_MASK
    };

    if (!hostAlloc(allocatedBlock)) {
        freeBlocks.insert(std::move(freeBlockNode));
        return Unexpected(Error::NoHostMemory);
    }

    // Split free block.
    auto freeBlockFirst = FreeBlock {
        .virtualBase = freeBlock.virtualBase,
        .size = allocatedBlock.virtualBase - freeBlock.virtualBase
    };
    if (freeBlockFirst.size)
        freeBlocks.insert(std::move(freeBlockFirst));

    auto freeBlockLast = FreeBlock {
        .virtualBase = allocatedBlock.virtualBase + allocatedBlock.size,
        .size = (freeBlock.virtualBase + freeBlock.size) - (allocatedBlock.virtualBase + allocatedBlock.size)
    };
    if (freeBlockLast.size)
        freeBlocks.insert(std::move(freeBlockLast));

    // Add the new allocated block.
    auto ret = allocatedBlocks.insert(allocatedBlock);
    DASHLE_ASSERT(ret.second);
    m_UsedMemory += allocatedBlock.size;
    return &*ret.first;
}

Expected<void> MemoryManager::free(uaddr vbase) {
    auto& allocatedBlocks = m_Data->allocatedBlocks;
    auto& freeBlocks = m_Data->freeBlocks;

    // Find allocated block.
    auto it = allocatedBlocks.find({.virtualBase = vbase});
    if (it == allocatedBlocks.end() || it->virtualBase != vbase)
        return Unexpected(Error::NotFound);

    // Handle merging with previous and next blocks.
    auto newFreeBlock = FreeBlock{
        .virtualBase = it->virtualBase,
        .size = it->size};

    const auto handleBlock = [&newFreeBlock=newFreeBlock](FreeBlockSet& freeBlocks, const FreeBlock& block, bool prev) {
        const auto freeBlockIt = freeBlocks.find(block);
        DASHLE_ASSERT(freeBlockIt != freeBlocks.end());
        // Update new free block.
        if (prev)
            newFreeBlock.virtualBase = freeBlockIt->virtualBase;
        newFreeBlock.size += freeBlockIt->size;
        // Erase old free block.
        freeBlocks.erase(freeBlockIt);
    };

    if (it != allocatedBlocks.begin()) {
        // Pick the previous block from the set, which is necessarily an allocated block. Since we sort by
        // virtual address, we know it's also the previous block in the address space.
        // By the way the allocation algorithm works, between two allocated blocks there can only be a single
        // free block, so either the previous block end matches with our block start, or it matches with the
        // start of a free block. In the latter case we will merge the two blocks.
        const auto prevAllocBlock = std::prev(it);
        const auto prevAllocBlockEnd = prevAllocBlock->virtualBase + prevAllocBlock->size;
        if (prevAllocBlockEnd != it->virtualBase) {
            const auto prevFreeBlock = FreeBlock{
                .virtualBase = prevAllocBlockEnd,
                .size = it->virtualBase - prevAllocBlockEnd};
            handleBlock(freeBlocks, prevFreeBlock, true);
        }
    } else {
        // Handle the case where this is the first allocated block of the set but not the first block in
        // the address space (ie. address space starts with a free block).
        if (it->virtualBase != 0u) {
            const auto firstFreeBlock = FreeBlock{
                .virtualBase = 0u,
                .size = it->virtualBase};
            handleBlock(freeBlocks, firstFreeBlock, true);
        }
    }

    if (const auto nextAllocBlock = std::next(it); nextAllocBlock != allocatedBlocks.end()) {
        // Same as above, except we check for the next allocated block.
        const auto thisAllocBlockEnd = it->virtualBase + it->size;
        if (thisAllocBlockEnd != nextAllocBlock->virtualBase) {
            const auto nextFreeBlock = FreeBlock{
                .virtualBase = thisAllocBlockEnd,
                .size = nextAllocBlock->virtualBase - thisAllocBlockEnd};
            handleBlock(freeBlocks, nextFreeBlock, false);
        }
    } else {
        // Handle the case where this is the last allocated block, but not the last in memory (ie. last
        // block is a free one).
        if ((it->virtualBase + it->size) != maxMemory()) {
            const auto thisAllocBlockEnd = it->virtualBase + it->size;
            const auto lastFreeBlock = FreeBlock{
                .virtualBase = thisAllocBlockEnd,
                .size = maxMemory() - thisAllocBlockEnd};
            handleBlock(freeBlocks, lastFreeBlock, false);
        }
    }

    // Free host space and add new free block.
    auto node = allocatedBlocks.extract(it);
    hostFree(node.value());
    m_UsedMemory -= node.value().size;
    freeBlocks.insert(newFreeBlock);
    return EXPECTED_VOID;
}

Expected<usize> MemoryManager::setFlags(uaddr vbase, usize flags) {
    return blockFromVAddr(vbase).and_then([vbase, flags](const AllocatedBlock* block) -> Expected<usize> {
        if (block->virtualBase == vbase) {
            const auto oldFlags = block->flags;
            const_cast<AllocatedBlock*>(block)->flags = flags;
            return oldFlags;
        }

        return Unexpected(Error::InvalidAddress);
    });
}