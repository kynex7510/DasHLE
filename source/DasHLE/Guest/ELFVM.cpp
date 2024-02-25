#include "DasHLE/Binary/ELF.h"
#include "DasHLE/Guest/ELFVM.h"

using namespace dashle;
using namespace dashle::guest;

namespace elf = dashle::binary::elf;

// ELFVM

struct LoadSegmentInfo {
    usize fileDataOffset; // Offset in the file for the first byte of this segment.
    usize fileDataSize; // Size of the data to read from file.
    usize memDataOffset; // Offset in memory for the first byte of this segment.
    usize allocOffset; // Offset in memory where this segment starts.
    usize allocSize; // Size for the memory allocation.
    usize permissions; // Permission flags.
};

static bool canRun(elf::ELF& elf) {
#if defined(DASHLE_HAS_GUEST_ARM)
    constexpr auto hasARM = true;
#else
    constexpr auto hasARM = false;
#endif // DASHLE_HAS_GUEST_ARM

#if defined(DASHLE_HAS_GUEST_AARCH64)
    constexpr auto hasAArch64 = true;
#else
    constexpr auto hasAArch64 = false;
#endif // DASHLE_HAS_GUEST_AARCH64

    return (!elf.is64Bits() && hasARM) || (elf.is64Bits() && hasAArch64);
}

template <typename T>
requires (OneOf<T, elf::Addr32, elf::Addr64>)
static void arrayRead(std::vector<usize>& entries, uaddr addr, usize numEntries) {
    entries.clear();
    const auto array = reinterpret_cast<const T*>(addr);
    for (auto i = 0u; i < numEntries; ++i) {
        const auto addr = array[i];
        if (addr != 0 && addr != -1)
            entries.push_back(addr);
    }
}

Expected<void> ELFVM::runInitializers() {
    if (!m_VM.holdsAny())
        return Unexpected(Error::InvalidOperation);

    for (auto vaddr : m_Initializers) {
        if (auto reason = m_VM->execute(vaddr); reason != VM_EXEC_SUCCESS) {
            DASHLE_UNREACHABLE("Init call failed (vaddr=0x{:X}, reason={})", vaddr, static_cast<u32>(reason));
        }
    }

    return EXPECTED_VOID;
}

Expected<void> ELFVM::runFinalizers() {
    if (!m_VM.holdsAny())
        return Unexpected(Error::InvalidOperation);

    for (auto vaddr : m_Finalizers) {
        if (auto reason = m_VM->execute(vaddr); reason != VM_EXEC_SUCCESS) {
            DASHLE_UNREACHABLE("Fini call failed (vaddr=0x{:X}, reason={})", vaddr, static_cast<u32>(reason));
        }
    }

    return EXPECTED_VOID;
}

ELFVM::ELFVM(std::shared_ptr<host::memory::MemoryManager> mem, std::shared_ptr<host::bridge::Bridge> bridge)
    : m_Mem(mem), m_Bridge(bridge) {}

Expected<void> ELFVM::loadBinary(std::vector<u8>&& buffer) {
    const auto virtualToHost = [this](uaddr vaddr) -> Expected<uaddr> {
        DASHLE_TRY_EXPECTED_CONST(block, m_Mem->blockFromVAddr(vaddr));
        return host::memory::virtualToHost(*block, vaddr);
    };

    const auto relocWriteVAddr = [this](uaddr addr, uaddr vaddr) {
        if (m_Elf.is64Bits()) {
            *reinterpret_cast<elf::Addr64*>(addr) = vaddr;
        } else {
            *reinterpret_cast<elf::Addr32*>(addr) = vaddr;
        }
    };

    const auto relocAddBinaryBase = [this](uaddr addr, uaddr base) {
        if (m_Elf.is64Bits()) {
            *reinterpret_cast<elf::Addr64*>(addr) += base;
        } else {
            *reinterpret_cast<elf::Addr32*>(addr) += base;
        }
    };

    // Parse ELF.
    DASHLE_TRY_EXPECTED_VOID(m_Elf.parse(std::move(buffer)));
    if (!canRun(m_Elf))
        return Unexpected(Error::InvalidArch);

    // Get ELF LOAD segments.
    std::vector<LoadSegmentInfo> loadSegmentsInfo;
    DASHLE_TRY_EXPECTED_CONST(loadSegments, m_Elf.segmentsOfType(elf::PT_LOAD));
    usize binaryAllocSize = 0u;

    for (const auto segment : loadSegments) {
        // vaddr is the offset in memory for the first byte of the segment.
        // allocOffset is vaddr but aligned down, so the offset in memory for the segment start.
        DASHLE_TRY_EXPECTED_CONST(allocOffset, segment->allocationOffset());
        DASHLE_TRY_EXPECTED_CONST(allocSize, segment->allocationSize());
        binaryAllocSize += allocSize;
        loadSegmentsInfo.emplace_back(LoadSegmentInfo {
            .fileDataOffset = segment->offset(),
            .fileDataSize = segment->filesz(),
            .memDataOffset = segment->vaddr() - allocOffset,
            .allocOffset = allocOffset,
            .allocSize = allocSize,
            .permissions = elf::wrapPermissionFlags(segment->flags())
        });
    }

    // Get binary base.
    // TODO: check alignment for aarch64.
    const usize alignment = 0x1000;
    DASHLE_TRY_EXPECTED_CONST(binaryBase, m_Mem->findFreeAddr(binaryAllocSize, alignment));
    DASHLE_LOG_LINE("Binary base: 0x{:X}", binaryBase);

    // Allocate and map each segment.
    for (const auto& segmentInfo : loadSegmentsInfo) {
        DASHLE_TRY_EXPECTED_CONST(block, m_Mem->allocate({
            .size = segmentInfo.allocSize,
            .alignment = alignment,
            .hint = binaryBase + segmentInfo.allocOffset,
            .flags = host::memory::flags::PERM_READ_WRITE | host::memory::flags::FORCE_HINT,
        }));

        std::copy(buffer.data() + segmentInfo.fileDataOffset,
            buffer.data() + segmentInfo.fileDataOffset + segmentInfo.fileDataSize,
            reinterpret_cast<u8*>(block->hostBase) + segmentInfo.memDataOffset);
    }

    // Handle relocations.
    for (const auto& reloc : m_Elf.relocs()) {
        DASHLE_TRY_EXPECTED_CONST(patchAddr, virtualToHost(binaryBase + reloc.patchOffset));

        if (reloc.kind == elf::RelocKind::Relative) {
            if (reloc.addend) {
                relocWriteVAddr(patchAddr, binaryBase + reloc.addend);
            } else {
                relocAddBinaryBase(patchAddr, binaryBase);
            }

            continue;
        }

        if (reloc.kind == elf::RelocKind::Symbol) {
            DASHLE_ASSERT_WRAPPER_CONST(symbol, reloc.symbol);
            DASHLE_TRY_EXPECTED_CONST(vaddr, m_Bridge->addressForSymbol(symbol));
            // We ignore the addend.
            relocWriteVAddr(patchAddr, vaddr);
            continue;
        }

        DASHLE_UNREACHABLE("Invalid relocation kind!");
    }

    // Set memory permissions.
    for (const auto& segmentInfo : loadSegmentsInfo) {
        DASHLE_TRY_EXPECTED_VOID(m_Mem->setFlags(binaryBase + segmentInfo.allocOffset, segmentInfo.permissions));
    }

    // Get initializers and finalizers.
    const auto initWrapper = m_Elf.initArrayInfo();
    if (initWrapper) {
        const auto& initArrayInfo = initWrapper.value();
        DASHLE_TRY_EXPECTED_CONST(initArray, virtualToHost(binaryBase + initArrayInfo.offset));
        if (m_Elf.is64Bits()) {
            arrayRead<elf::Addr64>(m_Initializers, initArray, initArrayInfo.size);
        } else {
            arrayRead<elf::Addr32>(m_Initializers, initArray, initArrayInfo.size);
        }
    }

    const auto finiWrapper = m_Elf.finiArrayInfo();
    if (finiWrapper) {
        const auto& finiArrayInfo = finiWrapper.value();
        DASHLE_TRY_EXPECTED_CONST(finiArray, virtualToHost(binaryBase + finiArrayInfo.offset));
        if (m_Elf.is64Bits()) {
            arrayRead<elf::Addr64>(m_Finalizers, finiArray, finiArrayInfo.size);
        } else {
            arrayRead<elf::Addr32>(m_Finalizers, finiArray, finiArrayInfo.size);
        }
    }

    // Instantiate VM.
    if (m_Elf.is64Bits()) {
        DASHLE_UNREACHABLE("Guest not supported!");
    } else {
#if defined(DASHLE_HAS_GUEST_ARM)
            m_VM = arm::ARMVM(m_Mem, m_Bridge, m_Elf.version());
#else
            DASHLE_UNREACHABLE("Guest not supported!");
#endif // DASHLE_HAS_GUEST_ARM
    }

    return EXPECTED_VOID;
}

Expected<void> ELFVM::loadBinary(const host::fs::path& path) {
    std::vector<u8> buffer;
    return host::fs::readFile(path, buffer).and_then([this, &buffer](){
        return loadBinary(std::move(buffer));
    });
}