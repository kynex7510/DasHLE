#include "DasHLE.h"
#include "ELF.h"

#include <fstream>
#include <vector>

using namespace dashle;
using namespace dashle::binary;

// Config

struct ConfigARM : public elf::Config32, public elf::ConfigLE {
    static constexpr auto ARCH = EM_ARM;
    static constexpr auto BINARY_TYPE = Type::ARM_LE;
};

struct ConfigAarch64 : public elf::Config64, public elf::ConfigLE {
    static constexpr auto ARCH = EM_AARCH64;
    static constexpr auto BINARY_TYPE = Type::Aarch64_LE;
};

ELF_ASSERT_CONFIG(ConfigARM);
ELF_ASSERT_CONFIG(ConfigAarch64);

// Binary

template <elf::ConfigType CFG>
static usize getSegmentsAllocSize(const elf::Segments<CFG> &segments) {
    usize allocSize = 0;

    for (const auto segment : segments) {
        if (segment->p_memsz < segment->p_filesz) {
            allocSize = 0;
            break;
        }

        if (segment->p_align > 1)
            allocSize += alignSize(segment->p_memsz, segment->p_align);
        else
            allocSize += segment->p_memsz;
    }

    return allocSize;
}

template <typename CFG>
Error Binary::loaderImpl(const std::span<const u8> buffer) {
    static_assert(elf::ConfigType<CFG>);
    auto header = elf::getHeader<CFG>(buffer);
    if (!header)
        return Error::InvalidObject;

    // Calculate allocation space for load segments.
    auto loadSegments = elf::getSegments<CFG>(header, PT_LOAD);
    if (loadSegments.empty())
        return Error::NoSegments;

    auto const regionSize = getSegmentsAllocSize<CFG>(loadSegments);
    if (!regionSize)
        return Error::InvalidSegments;

    // Allocate memory and map segments.
    auto p = m_Allocator.allocate(0u, regionSize, false);
    if (!p)
        return Error::NoMemory;

    /*
    if (auto block = mman.blockFromVAddr(p)) {
        p = block->virtualToHost(p);
    }

    if (!p) {
        m_Allocator.free(p);
        return Error::NoMemory;
    }
    */

    for (auto const segment : loadSegments) {
        std::copy(
            buffer.data() + segment->p_offset,
            buffer.data() + segment->p_offset + segment->p_filesz,
            reinterpret_cast<u8 *>(p) + segment->p_vaddr);
    }

    // Apply relocations.
    // TODO

    m_Type = CFG::BINARY_TYPE;
    m_CodeBuffer = std::span{reinterpret_cast<u8 *>(p), regionSize};
    return Error::Success;
}

Error Binary::load(const stdfs::path &path) {
    // Read binary.
    std::vector<u8> buffer;
    std::ifstream fileHandle(path, std::ios::ate | std::ios::binary);
    if (!fileHandle.is_open())
        return Error::OpenFailed;

    buffer.resize(fileHandle.tellg());
    fileHandle.seekg(0, fileHandle.beg);
    fileHandle.read(reinterpret_cast<char *>(buffer.data()), buffer.size());

    // Load binary in memory.
    Error error = Error::UnknownArch;

#define CFG_ARCH_CASE(cfg)               \
    case cfg::ARCH:                      \
        error = loaderImpl<cfg>(buffer); \
        break;

    switch (elf::detectArch(buffer)) {
        CFG_ARCH_CASE(ConfigARM);
        CFG_ARCH_CASE(ConfigAarch64);
    }

#undef CFG_ARCH_BASE

    return error;
}

std::string binary::errorAsString(Error error) {
#define ERROR_CASE(s) \
    case Error::s:    \
        return #s;

    switch (error) {
        ERROR_CASE(Success);
        ERROR_CASE(OpenFailed);
        ERROR_CASE(UnknownArch);
        ERROR_CASE(InvalidObject);
        ERROR_CASE(NoSegments);
        ERROR_CASE(InvalidSegments);
        ERROR_CASE(NoMemory);
        ERROR_CASE(RelocationFailed);
    }

#undef ERROR_CASE

    return {};
}

std::string binary::typeAsString(Type type) {
#define TYPE_CASE(s) \
    case Type::s:    \
        return #s;

    switch (type) {
        TYPE_CASE(ARM_LE);
        TYPE_CASE(Aarch64_LE);
    }

#undef TYPE_CASE

    return {};
}